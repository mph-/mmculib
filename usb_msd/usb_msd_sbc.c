/* (SBC) SCSI Block Commands 

   This uses the USB bulk transport layer (BOT) for communicating
   with the USB host and the logical unit (LUN) for interfacing
   with a mass storage device.  */

#include "usb_trace.h"
#include "usb_bot.h"
#include "usb_msd_sbc.h"
#include "usb_msd_lun.h"
#include "usb_sbc_defs.h"
#include "byteorder.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * \name Possible states of a SBC command.
 * 
 */
typedef enum
{
    SBC_STATE_INIT = 0,
    SBC_STATE_READ,
    SBC_STATE_READ_WAIT,
    SBC_STATE_WRITE,
    SBC_STATE_WRITE_WAIT
} sbc_state_t;


static sbc_state_t sbc_state;
static uint8_t sbc_tmp_buffer[MSD_BLOCK_SIZE_MAX];


/**
 * Handles an INQUIRY command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of USB_BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return  Operation result code (SUCCESS, ERROR, INCOMPLETE, or PARAMETER)
 * 
 */
static usb_bot_status_t
sbc_inquiry (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    usb_bot_status_t bResult = USB_BOT_STATUS_INCOMPLETE;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;

    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:Inquiry\n");
        sbc_state = SBC_STATE_WRITE;
        
        // Change additional length field of inquiry data
        pLun->sInquiryData.bAdditionalLength
            = (uint8_t) (pCommandState->dLength - 5);
        /* Fall through...  */

    case SBC_STATE_WRITE:
        // Start write operation
        usb_bot_write (&pLun->sInquiryData, pCommandState->dLength, pTransfer);
        sbc_state = SBC_STATE_WRITE_WAIT;
        break;

    case SBC_STATE_WRITE_WAIT:
        bResult = usb_bot_transfer_status (pTransfer);
        if (bResult == USB_BOT_STATUS_SUCCESS)
            pCommandState->dLength -= usb_bot_transfer_bytes (pTransfer);
        else if (bResult == USB_BOT_STATUS_ERROR_USB_WRITE)
            TRACE_ERROR (USB_MSD_SBC, "SBC:Inquiry error\n");
        break;

    default:
        TRACE_ERROR (USB_MSD_SBC, "SBC: Bad state\n");
        break;
    }

    return bResult;
}


/**
 * Performs a READ CAPACITY (10) command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of USB_BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return  Operation result code (SUCCESS, ERROR, INCOMPLETE, or PARAMETER)
 * 
 */
static usb_bot_status_t 
sbc_read_capacity10 (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    usb_bot_status_t bResult = USB_BOT_STATUS_INCOMPLETE;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;

    // Identify current command state
    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:RdCapacity\n");
        sbc_state = SBC_STATE_WRITE;
        /* Fall through...  */

    case SBC_STATE_WRITE:
        // Start the write operation
        usb_bot_write (&pLun->sReadCapacityData,
                       pCommandState->dLength, pTransfer);
        sbc_state = SBC_STATE_WRITE_WAIT;
        break;

    case SBC_STATE_WRITE_WAIT:
        bResult = usb_bot_transfer_status (pTransfer);
        if (bResult == USB_BOT_STATUS_SUCCESS)
            pCommandState->dLength -= usb_bot_transfer_bytes (pTransfer);
        else if (bResult == USB_BOT_STATUS_ERROR_USB_WRITE)
            TRACE_ERROR (USB_MSD_SBC, "SBC:Capacity error\n");
        break;

    default:
        TRACE_ERROR (USB_MSD_SBC, "SBC: Bad state\n");
        break;
    }

    return bResult;
}


/**
 * Performs a WRITE (10) command on the specified LUN.
 * 
 * The data to write is first received from the USB host and then
 * actually written on the media.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of USB_BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE, or PARAMETER)
 * 
 */
static usb_bot_status_t
sbc_write10 (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    lun_status_t bStatus;
    usb_bot_status_t bResult = USB_BOT_STATUS_INCOMPLETE;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;
    S_sbc_write_10 *pCommand = (S_sbc_write_10 *) pCommandState->sCbw.pCommand;
    usb_msd_lun_addr_t addr;

    addr = DWORDB (pCommand->pLogicalBlockAddress);

    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:Write %u @%u\n", 
                    (unsigned int)pCommandState->dLength,
                    (unsigned int) addr);
        sbc_state = SBC_STATE_READ;
        /* Fall through...  */

    case SBC_STATE_READ:
        TRACE_DEBUG (USB_MSD_SBC, "SBC:BOT read start\n");
        // Read one block of data sent by the host
        usb_bot_read (sbc_tmp_buffer, pLun->block_bytes, pTransfer);
        sbc_state = SBC_STATE_READ_WAIT;
        break;
        
    case SBC_STATE_READ_WAIT:
        bResult = usb_bot_transfer_status (pTransfer);
        if (bResult == USB_BOT_STATUS_SUCCESS)
        {
            TRACE_DEBUG (USB_MSD_SBC, "SBC:BOT read done\n");
            sbc_state = SBC_STATE_WRITE;            
        }
        break;
        
    case SBC_STATE_WRITE:
    case SBC_STATE_WRITE_WAIT:
        // Write the block to the media
        TRACE_DEBUG (USB_MSD_SBC, "SBC:LUN write\n");
        bStatus = lun_write (pLun, addr, sbc_tmp_buffer, 1);

        if (bStatus != LUN_STATUS_SUCCESS)
        {
            bResult = USB_BOT_STATUS_ERROR_LUN_WRITE;
        }
        else
        {
            TRACE_DEBUG (USB_MSD_SBC, "SBC:LUN write done\n");
            // Update transfer length and block address
            pCommandState->dLength -=  pLun->block_bytes;
            STORE_DWORDB (addr + 1, pCommand->pLogicalBlockAddress);
            sbc_state = SBC_STATE_READ;
            if (pCommandState->dLength != 0)
                bResult = USB_BOT_STATUS_INCOMPLETE;
        }
        break;
    }

    return bResult;
}



/**
 * Performs a READ (10) command on specified LUN.
 * 
 * The data is first read from the media and then sent to the USB host.
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of USB_BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return  Operation result code (SUCCESS, ERROR, INCOMPLETE, or PARAMETER)
 * 
 */
static usb_bot_status_t
sbc_read10 (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    lun_status_t bStatus;
    usb_bot_status_t bResult = USB_BOT_STATUS_INCOMPLETE;
    S_sbc_read_10 *pCommand = (S_sbc_read_10 *) pCommandState->sCbw.pCommand;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;
    usb_msd_lun_addr_t addr;

    addr = DWORDB (pCommand->pLogicalBlockAddress);

    /* dLength should be a multiple of the LUN block length.  */

    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:Read %u @%u\n",
                    (unsigned int)pCommandState->dLength,
                    (unsigned int)addr);
        sbc_state = SBC_STATE_READ;
        /* Fall through ...  */

    case SBC_STATE_READ:
    case SBC_STATE_READ_WAIT:
        TRACE_DEBUG (USB_MSD_SBC, "SBC:LUN read start\n");

        // Read one block of data from the media
        bStatus = lun_read (pLun, addr, sbc_tmp_buffer, 1);
        
        if (bStatus != LUN_STATUS_SUCCESS)
        {
            bResult = USB_BOT_STATUS_ERROR_LUN_READ;
        }
        else
        {
            sbc_state = SBC_STATE_WRITE;
        }
        break;
        
    case SBC_STATE_WRITE:
        TRACE_DEBUG (USB_MSD_SBC, "SBC:BOT write start\n");
        
        // Send the block to the host
        usb_bot_write (sbc_tmp_buffer, pLun->block_bytes, pTransfer);
        sbc_state = SBC_STATE_WRITE_WAIT;
        break;
        
    case SBC_STATE_WRITE_WAIT:
        bResult = usb_bot_transfer_status (pTransfer);
        if (bResult == USB_BOT_STATUS_SUCCESS)
        {
            TRACE_DEBUG (USB_MSD_SBC, "SBC:BOT write done\n");
            
            // Update transfer length and block address
            pCommandState->dLength -= usb_bot_transfer_bytes (pTransfer);
            STORE_DWORDB (addr + 1, pCommand->pLogicalBlockAddress);
            
            sbc_state = SBC_STATE_READ;
            
            if (pCommandState->dLength != 0)
                bResult = USB_BOT_STATUS_INCOMPLETE;

        }
        break;
    }

    return bResult;
}


/**
 * Performs a MODE SENSE (6) command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of USB_BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE, or PARAMETER)
 * 
 */
static usb_bot_status_t
sbc_mode_sense6 (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    usb_bot_status_t bResult = USB_BOT_STATUS_INCOMPLETE;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;
    S_sbc_mode_parameter_header_6 sModeParameterHeader6 =
        {
            //!< Length of mode page data is 0x03
            sizeof (S_sbc_mode_parameter_header_6) - 1,
            //!< Direct-access block device
            SBC_MEDIUM_TYPE_DIRECT_ACCESS_BLOCK_DEVICE,
            //!< Reserved bits
            0,
            //!< DPO/FUA not supported
            false,
            //!< Reserved bits
            0,
            //!< Write-protect status
            pLun->write_protect,
            //!< No block descriptor
            0
        };

    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:ModeSense\n");
        sbc_state = SBC_STATE_WRITE;
        /* Fall through...  */

    case SBC_STATE_WRITE:
        // Start transfer
        usb_bot_write (&sModeParameterHeader6, pCommandState->dLength, pTransfer);
        sbc_state = SBC_STATE_WRITE_WAIT;
        break;
    
    case SBC_STATE_WRITE_WAIT:
        bResult = usb_bot_transfer_status (pTransfer);
        if (bResult == USB_BOT_STATUS_SUCCESS)
            pCommandState->dLength -= usb_bot_transfer_bytes (pTransfer);
        else if (bResult == USB_BOT_STATUS_ERROR_USB_WRITE)
            TRACE_ERROR (USB_MSD_SBC, "SBC:Write error\n");            
        break;

    default:
        TRACE_ERROR (USB_MSD_SBC, "SBC: Bad state\n");
        break;
    }

    return bResult;
}


/**
 * Performs a REQUEST SENSE command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of USB_BOT_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE, or PARAMETER)
 * 
 */
static usb_bot_status_t
sbc_request_sense (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    usb_bot_status_t bResult = USB_BOT_STATUS_INCOMPLETE;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;

    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:ReqSense\n");
        sbc_state = SBC_STATE_WRITE;
        /* Fall through ...  */

    case SBC_STATE_WRITE:
        usb_bot_write (&pLun->sRequestSenseData, pCommandState->dLength, pTransfer);
        sbc_state = SBC_STATE_WRITE_WAIT;
        break;
    
    case SBC_STATE_WRITE_WAIT:
        bResult = usb_bot_transfer_status (pTransfer);
        if (bResult == USB_BOT_STATUS_SUCCESS)
            pCommandState->dLength -= usb_bot_transfer_bytes (pTransfer);
        else if (bResult == USB_BOT_STATUS_ERROR_USB_WRITE)
            TRACE_ERROR (USB_MSD_SBC, "SBC:Write error\n");
        break;

    default:
        TRACE_ERROR (USB_MSD_SBC, "SBC: Bad state\n");
        break;
    }

    return bResult;
}


/**
 * Performs a TEST UNIT READY COMMAND command.
 * 
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE, or PARAMETER)
 * 
 */
static usb_bot_status_t
sbc_test_unit_ready (usb_msd_lun_t *pLun)
{
    msd_status_t status;

    TRACE_INFO (USB_MSD_SBC, "SBC:TstUnitRdy\n");
    
    status = lun_status_get (pLun);

    switch (status)
    {
    case MSD_STATUS_READY:
        TRACE_INFO (USB_MSD_SBC, "SBC:Ready\n");
        return USB_BOT_STATUS_SUCCESS;
        
    case MSD_STATUS_BUSY:
        TRACE_INFO (USB_MSD_SBC, "SBC:Busy\n");
        return USB_BOT_STATUS_ERROR_LUN_BUSY;

    default:
    case MSD_STATUS_NODEVICE:
        TRACE_INFO (USB_MSD_SBC, "SBC:?\n");
        return USB_BOT_STATUS_ERROR_LUN_NODEVICE;
    }
}


/**
 * Return information about the transfer length and direction expected
 * by the device for a particular command.
 * 
 * \param   pCbw        Pointer to CBW
 * \param   pLength     Expected length of the data transfer
 * \param   pType       Expected direction of data transfer
 * \return  Command support status
 */
bool
sbc_get_command_information (usb_msd_cbw_t *pCbw, uint32_t *pLength, uint8_t *pType)
{
    void *pCommand = pCbw->pCommand;
    S_sbc_command *pSbcCommand = (S_sbc_command *) pCommand;
    bool isCommandSupported = true;
    usb_msd_lun_t *pLun;

    pLun = lun_get (pCbw->bCBWLUN);
    if (!pLun)
        return false;

    // Identify command
    switch (pSbcCommand->bOperationCode)
    {
    case SBC_INQUIRY:
        *pType = USB_BOT_DEVICE_TO_HOST;
    
        // Allocation length is stored in big-endian format
        *pLength = WORDB (pSbcCommand->sInquiry.pAllocationLength);
        break;
    
    case SBC_MODE_SENSE_6:
        // Linux requests 192 bytes but we only supply 4
        *pType = USB_BOT_DEVICE_TO_HOST;
        *pLength = MIN (sizeof (S_sbc_mode_parameter_header_6),
                        pSbcCommand->sModeSense6.bAllocationLength);
    
        // Only "return all pages" command is supported
        if (pSbcCommand->sModeSense6.bPageCode != SBC_PAGE_RETURN_ALL)
        {
            // Unsupported page, Windows sends this
            TRACE_INFO (USB_MSD_SBC, "SBC:Bad page code 0X%02x\n",
                        pSbcCommand->sModeSense6.bPageCode);
            isCommandSupported = false;
            *pLength = 0;
        }
        break;
    
    case SBC_PREVENT_ALLOW_MEDIUM_REMOVAL:
        *pType = USB_BOT_NO_TRANSFER;
        break;
    
    case SBC_REQUEST_SENSE:
        *pType = USB_BOT_DEVICE_TO_HOST;
        *pLength = pSbcCommand->sRequestSense.bAllocationLength;
        break;
    
    case SBC_TEST_UNIT_READY:
        *pType = USB_BOT_NO_TRANSFER;
        break;
    
    case SBC_READ_CAPACITY_10:
        *pType = USB_BOT_DEVICE_TO_HOST;
        *pLength = sizeof (S_sbc_read_capacity_10_data);
        break;
    
    case SBC_READ_10:
        *pType = USB_BOT_DEVICE_TO_HOST;
        *pLength = WORDB (pSbcCommand->sRead10.pTransferLength)
            * pLun->block_bytes;
        break;
    
    case SBC_WRITE_10:
        *pType = USB_BOT_HOST_TO_DEVICE;
        *pLength = WORDB (pSbcCommand->sWrite10.pTransferLength)
            * pLun->block_bytes;
        break;
    
    case SBC_VERIFY_10:
        *pType = USB_BOT_NO_TRANSFER;
        break;
    
    default:
        isCommandSupported = false;
    }

    // If length is 0, no transfer is expected
    if (*pLength == 0)
        *pType = USB_BOT_NO_TRANSFER;

    return isCommandSupported;
}


/**
 * Processes a SBC command by dispatching it to a subfunction.
 * 
 * \param   pCommandState   Pointer to the current command state
 * \return  Operation result code
 * 
 */
usb_bot_status_t
sbc_process_command (S_usb_bot_command_state *pCommandState)
{
    usb_bot_status_t bResult = USB_BOT_STATUS_INCOMPLETE;
    S_sbc_command *pCommand;
    usb_msd_cbw_t *pCbw = &pCommandState->sCbw;
    usb_msd_lun_t *pLun;

    /* Need to cast array containing command to union of command
       structures.  */
    pCommand = (S_sbc_command *) pCommandState->sCbw.pCommand;

    // If lun in error should we do bad parameter handling?
    pLun = lun_get (pCbw->bCBWLUN);
    if (!pLun)
        return USB_BOT_STATUS_ERROR_CBW_PARAMETER;

    // Identify command
    switch (pCommand->bOperationCode)
    {
    case SBC_READ_10:
        bResult = sbc_read10 (pLun, pCommandState);
        break;

    case SBC_WRITE_10:
        bResult = sbc_write10 (pLun, pCommandState);
        break;

    case SBC_READ_CAPACITY_10:
        bResult = sbc_read_capacity10 (pLun, pCommandState);
        break;

    case SBC_VERIFY_10:
        TRACE_INFO (USB_MSD_SBC, "SBC:Verify\n");
        // Nothing to do
        bResult = USB_BOT_STATUS_SUCCESS;
        break;

    case SBC_INQUIRY:
        bResult = sbc_inquiry (pLun, pCommandState);
        break;

    case SBC_MODE_SENSE_6:
        bResult = sbc_mode_sense6 (pLun, pCommandState);
        break;

    case SBC_TEST_UNIT_READY:
        bResult = sbc_test_unit_ready (pLun);
        break;

    case SBC_REQUEST_SENSE:
        bResult = sbc_request_sense (pLun, pCommandState);
        break;

    case SBC_PREVENT_ALLOW_MEDIUM_REMOVAL:
        TRACE_INFO (USB_MSD_SBC, "SBC:PrevAllowRem\n");
        // Nothing to do
        bResult = USB_BOT_STATUS_SUCCESS;
        break;

    default:
        bResult = USB_BOT_STATUS_ERROR_CBW_PARAMETER;
    }

    switch (bResult)
    {
    case USB_BOT_STATUS_ERROR_CBW_FORMAT:
    case USB_BOT_STATUS_ERROR_CBW_PARAMETER:
        TRACE_INFO (USB_MSD_SBC, "SBC:Bad command 0x%02x\n",
                    pCbw->pCommand[0]);

        lun_sense_data_update (pLun, SBC_SENSE_KEY_ILLEGAL_REQUEST,
                               SBC_ASC_INVALID_COMMAND_OPERATION_CODE,
                               /* SBC_ASC_INVALID_FIELD_IN_CDB, */
                               0);
        break;
    
    case USB_BOT_STATUS_ERROR_USB_READ:
    case USB_BOT_STATUS_ERROR_USB_WRITE:
        TRACE_ERROR (USB_MSD_SBC, "SBC:Command failed\n");
        lun_sense_data_update (pLun, SBC_SENSE_KEY_MEDIUM_ERROR,
                               SBC_ASC_INVALID_FIELD_IN_CDB, 0);
        break;

    case USB_BOT_STATUS_ERROR_LUN_READ:
        TRACE_ERROR (USB_MSD_SBC, "SBC:LUN read error\n");
        lun_sense_data_update (pLun, SBC_SENSE_KEY_NOT_READY,
                               SBC_ASC_LOGICAL_UNIT_NOT_READY, 0);
        break;

    case USB_BOT_STATUS_ERROR_LUN_WRITE:
        TRACE_ERROR (USB_MSD_SBC, "SBC:LUN write error\n");
        lun_sense_data_update (pLun, SBC_SENSE_KEY_NOT_READY, 0, 0);
        break;

    case USB_BOT_STATUS_ERROR_LUN_BUSY:
        TRACE_ERROR (USB_MSD_SBC, "SBC:LUN busy\n");
        lun_sense_data_update (pLun, SBC_SENSE_KEY_NOT_READY, 0, 0);
        break;

    case USB_BOT_STATUS_ERROR_LUN_NODEVICE:
        TRACE_ERROR (USB_MSD_SBC, "SBC:LUN not available\n");
        lun_sense_data_update (pLun, SBC_SENSE_KEY_NOT_READY, 0, 0);
        break;

    case USB_BOT_STATUS_INCOMPLETE:
        lun_sense_data_update (pLun, SBC_SENSE_KEY_NO_SENSE, 0, 0);
        break;

    case USB_BOT_STATUS_SUCCESS:
        TRACE_DEBUG (USB_MSD_SBC, "SBC:Command complete\n");
        lun_sense_data_update (pLun, SBC_SENSE_KEY_NO_SENSE, 0, 0);
        break;
    }

    if (bResult != USB_BOT_STATUS_INCOMPLETE
        && bResult != USB_BOT_STATUS_SUCCESS)
        usb_bot_error_log (bResult);

    /* If finished operation or have error, reset state.  */
    if (bResult != USB_BOT_STATUS_INCOMPLETE)
        sbc_state = SBC_STATE_INIT;

    return bResult;
}


void
sbc_lun_init (msd_t *msd)
{
    lun_init (msd);
}


uint8_t
sbc_lun_num_get (void)
{
    return lun_num_get ();
}


void
sbc_reset (void)
{
    sbc_state = SBC_STATE_INIT;
}


void  sbc_lun_write_protect_set (uint8_t lun_id, bool enable)
{
    usb_msd_lun_t *pLun;
    
    pLun = lun_get (lun_id);
    if (!pLun)
        return;
    
    lun_write_protect_set (pLun, enable);
}

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
    SBC_STATE_WAIT_READ,
    SBC_STATE_WRITE,
    SBC_STATE_WAIT_WRITE
} sbc_state_t;


static sbc_state_t sbc_state;
static uint8_t sbc_tmp_buffer[MSD_BLOCK_SIZE_MAX];


/**
 * Header for the mode pages data
 * 
 */
static const S_sbc_mode_parameter_header_6 sModeParameterHeader6 =
{
    sizeof (S_sbc_mode_parameter_header_6) - 1,  //!< Length of mode page data is 0x03
    SBC_MEDIUM_TYPE_DIRECT_ACCESS_BLOCK_DEVICE, //!< Direct-access block device
    0,                                          //!< Reserved bits
    false,                                      //!< DPO/FUA not supported
    0,                                          //!< Reserved bits
    false,                                      //!< Medium is not write-protected
    0                                           //!< No block descriptor
};


static sbc_status_t
sbc_status (usb_bot_status_t status)
{
    switch (status)
    {
    case USB_BOT_STATUS_SUCCESS:
        return SBC_STATUS_SUCCESS;

    default:
        return SBC_STATUS_ERROR;
    }
}



/**
 * Handles an INQUIRY command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of SBC_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return  Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static sbc_status_t
sbc_inquiry (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    sbc_status_t bResult = SBC_STATUS_INCOMPLETE;
    usb_bot_status_t bStatus;
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
        bStatus = usb_bot_write (&pLun->sInquiryData, pCommandState->dLength,
                                 pTransfer);

        if (bStatus != USB_BOT_STATUS_SUCCESS)
        {
            bResult = SBC_STATUS_ERROR;
            TRACE_ERROR (USB_MSD_SBC, "SBC:Inquiry error\n");
        }
        else
            sbc_state = SBC_STATE_WAIT_WRITE;
        break;

    case SBC_STATE_WAIT_WRITE:
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;
            bResult = sbc_status (pTransfer->bStatus);
            pCommandState->dLength -= pTransfer->dBytesTransferred;
        }
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
 * times to complete. A result code of SBC_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return  Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static sbc_status_t 
sbc_read_capacity10 (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    sbc_status_t bResult = SBC_STATUS_INCOMPLETE;
    usb_bot_status_t bStatus;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;

    // Identify current command state
    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:RdCapacity (10)\n");
        sbc_state = SBC_STATE_WRITE;
        /* Fall through...  */

    case SBC_STATE_WRITE:
        // Start the write operation
        bStatus = usb_bot_write (&pLun->sReadCapacityData,
                                 pCommandState->dLength, pTransfer);
    
        if (bStatus == USB_BOT_STATUS_SUCCESS)
            sbc_state = SBC_STATE_WAIT_WRITE;
        else
            bResult = SBC_STATUS_ERROR;
        break;

    case SBC_STATE_WAIT_WRITE:
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;
            bResult = sbc_status (pTransfer->bStatus);
            pCommandState->dLength -= pTransfer->dBytesTransferred;
        }
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
 * times to complete. A result code of SBC_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static sbc_status_t
sbc_write10 (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    usb_bot_status_t bStatus;
    sbc_status_t bResult = SBC_STATUS_INCOMPLETE;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;
    S_sbc_write_10 *pCommand = (S_sbc_write_10 *) pCommandState->sCbw.pCommand;
    usb_msd_lun_addr_t addr;

    addr = DWORDB (pCommand->pLogicalBlockAddress);

    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:Write (%d) [%u]\n", 
               (unsigned int) addr,
               (unsigned int)pCommandState->dLength);
        sbc_state = SBC_STATE_READ;
        /* Fall through...  */

    case SBC_STATE_READ:
        TRACE_DEBUG (USB_MSD_SBC, "SBC:BOT read start\n");
        // Read one block of data sent by the host
        bStatus = usb_bot_read (sbc_tmp_buffer, pLun->block_bytes,
                                pTransfer);

        if (bStatus != USB_BOT_STATUS_SUCCESS)
        {
            TRACE_ERROR (USB_MSD_SBC, "SBC:BOT read error\n");
            lun_update_sense_data (pLun, SBC_SENSE_KEY_HARDWARE_ERROR, 0, 0);
            bResult = SBC_STATUS_ERROR;
        }
        else
            sbc_state = SBC_STATE_WAIT_READ;
        break;
        
    case SBC_STATE_WAIT_READ:
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;

            if (pTransfer->bStatus != USB_BOT_STATUS_SUCCESS)
            {
                TRACE_ERROR (USB_MSD_SBC, "SBC:BOT read error\n");
                lun_update_sense_data (pLun, SBC_SENSE_KEY_HARDWARE_ERROR,
                                       0, 0);
                bResult = SBC_STATUS_ERROR;
            }
            else
            {
                TRACE_DEBUG (USB_MSD_SBC, "SBC:BOT read done\n");
                sbc_state = SBC_STATE_WRITE;
            }
        }
        break;
        
    case SBC_STATE_WRITE:
        // Write the block to the media
        TRACE_DEBUG (USB_MSD_SBC, "SBC:LUN write\n");
        bStatus = lun_write (pLun, addr, sbc_tmp_buffer, 1);
        pTransfer->bSemaphore++;
        pTransfer->bStatus = bStatus;
        pTransfer->dBytesTransferred = pLun->block_bytes;
        pTransfer->dBytesRemaining = 0;
        
        if (bStatus != LUN_STATUS_SUCCESS)
        {
            TRACE_ERROR (USB_MSD_SBC, "SBC:LUN write error\n");
            lun_update_sense_data (pLun, SBC_SENSE_KEY_NOT_READY,
                                   0, 0);
            bResult = SBC_STATUS_ERROR;
        }
        else
        {
            // Prepare next state
            sbc_state = SBC_STATE_WAIT_WRITE;
        }
        break;
        
    case SBC_STATE_WAIT_WRITE:
        // Check semaphore value  (If this is not set then there is
        // an error since lun_write is synchronous).
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;

            if (pTransfer->bStatus != LUN_STATUS_SUCCESS)
            {
                TRACE_ERROR (USB_MSD_SBC, "SBC:LUN write error\n");
                lun_update_sense_data (pLun, SBC_SENSE_KEY_RECOVERED_ERROR,
                                       SBC_ASC_TOO_MUCH_WRITE_DATA,
                                       0);
                bResult = SBC_STATUS_ERROR;
            }
            else
            {
                TRACE_DEBUG (USB_MSD_SBC, "SBC:LUN write done\n");
                // Update transfer length and block address
                pCommandState->dLength -= pTransfer->dBytesTransferred;
                STORE_DWORDB (addr + 1, pCommand->pLogicalBlockAddress);

                sbc_state = SBC_STATE_READ;
                if (pCommandState->dLength == 0)
                    bResult = SBC_STATUS_SUCCESS;
            }
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
 * times to complete. A result code of SBC_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return  Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static sbc_status_t
sbc_read10 (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    usb_bot_status_t bStatus;
    sbc_status_t bResult = SBC_STATUS_INCOMPLETE;
    S_sbc_read_10 *pCommand = (S_sbc_read_10 *) pCommandState->sCbw.pCommand;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;
    usb_msd_lun_addr_t addr;

    addr = DWORDB (pCommand->pLogicalBlockAddress);

    /* dLength should be a multiple of the LUN block length.  */

    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:Read (%u) [%u]\n",
                   (unsigned int)addr,
                   (unsigned int)pCommandState->dLength);
        sbc_state = SBC_STATE_READ;
        /* Fall through ...  */

    case SBC_STATE_READ:
        TRACE_DEBUG (USB_MSD_SBC, "SBC:LUN read start\n");

        // Read one block of data from the media
        bStatus = lun_read (pLun, addr, sbc_tmp_buffer, 1);
        // This code assumes async LUN I/O but currently it is only sync.
        pTransfer->bSemaphore++;
        pTransfer->bStatus = bStatus;
        pTransfer->dBytesTransferred = pLun->block_bytes;
        pTransfer->dBytesRemaining = 0;
        
        if (bStatus != LUN_STATUS_SUCCESS)
        {
            TRACE_ERROR (USB_MSD_SBC, "SBC:LUN read error\n");
            lun_update_sense_data (pLun, SBC_SENSE_KEY_NOT_READY,
                                   SBC_ASC_LOGICAL_UNIT_NOT_READY,
                                   0);
            bResult = SBC_STATUS_ERROR;
        }
        else
        {
            // Move to next command state
            sbc_state = SBC_STATE_WAIT_READ;
        }
        break;
        
    case SBC_STATE_WAIT_READ:
        // Check semaphore value  (If this is not set then there is
        // an error since lun_read is synchronous).
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;

            if (pTransfer->bStatus != LUN_STATUS_SUCCESS)
            {
                TRACE_ERROR (USB_MSD_SBC, "SBC:LUN read error\n");
                lun_update_sense_data (pLun, SBC_SENSE_KEY_RECOVERED_ERROR,
                                       SBC_ASC_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE,
                                       0);
                bResult = SBC_STATUS_ERROR;
            }
            else
            {
                TRACE_DEBUG (USB_MSD_SBC, "SBC:LUN read done\n");
                sbc_state = SBC_STATE_WRITE;
            }
        }
        break;
        
    case SBC_STATE_WRITE:
        TRACE_DEBUG (USB_MSD_SBC, "SBC:BOT write start\n");
        
        // Send the block to the host
        // Partial blocks are never written
        bStatus = usb_bot_write (sbc_tmp_buffer,
                                 pTransfer->dBytesTransferred, pTransfer);
        
        if (bStatus != USB_BOT_STATUS_SUCCESS)
        {
            TRACE_ERROR (USB_MSD_SBC, "SBC:BOT write error\n");
            lun_update_sense_data (pLun, SBC_SENSE_KEY_HARDWARE_ERROR,
                                   0, 0);
            bResult = SBC_STATUS_ERROR;
        }
        else
        {
            sbc_state = SBC_STATE_WAIT_WRITE;
        }
        break;
        
    case SBC_STATE_WAIT_WRITE:
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;

            if (pTransfer->bStatus != USB_BOT_STATUS_SUCCESS)
            {
                TRACE_ERROR (USB_MSD_SBC, "SBC:BOT write error\n");
                lun_update_sense_data (pLun, SBC_SENSE_KEY_HARDWARE_ERROR,
                                       0, 0);
                bResult = SBC_STATUS_ERROR;
            }
            else
            {
                TRACE_DEBUG (USB_MSD_SBC, "SBC:BOT write done\n");

                // Update transfer length and block address
                pCommandState->dLength -= pTransfer->dBytesTransferred;
                STORE_DWORDB (addr + 1, pCommand->pLogicalBlockAddress);

                sbc_state = SBC_STATE_READ;

                if (pCommandState->dLength == 0)
                    bResult = SBC_STATUS_SUCCESS;
            }
        }
        break;
        
    }

    return bResult;
}


/**
 * Performs a MODE SENSE (6) command.
 * 
 * This function operates asynchronously and must be called multiple
 * times to complete. A result code of SBC_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static sbc_status_t
sbc_mode_sense6 (usb_msd_lun_t *pLun __unused__, S_usb_bot_command_state *pCommandState)
{
    sbc_status_t bResult = SBC_STATUS_INCOMPLETE;
    usb_bot_status_t bStatus;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;

    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:ModeSense\n");
        sbc_state = SBC_STATE_WRITE;
        /* Fall through...  */

    case SBC_STATE_WRITE:
        // Start transfer
        bStatus = usb_bot_write (&sModeParameterHeader6, pCommandState->dLength,
                                 pTransfer);
    
        if (bStatus != USB_BOT_STATUS_SUCCESS)
        {
            TRACE_ERROR (USB_MSD_SBC, "SBC:Write error\n");
            bResult = SBC_STATUS_ERROR;
        }
        else
            sbc_state = SBC_STATE_WAIT_WRITE;
        break;
    
    case SBC_STATE_WAIT_WRITE:
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;
            bResult = sbc_status (pTransfer->bStatus);
            pCommandState->dLength -= pTransfer->dBytesTransferred;
        }
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
 * times to complete. A result code of SBC_STATUS_INCOMPLETE indicates
 * that at least another call of the method is necessary.
 * 
 * \param   pCommandState   Current state of the command
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static sbc_status_t
sbc_request_sense (usb_msd_lun_t *pLun, S_usb_bot_command_state *pCommandState)
{
    sbc_status_t bResult = SBC_STATUS_INCOMPLETE;
    usb_bot_status_t bStatus;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;

    switch (sbc_state)
    {
    case SBC_STATE_INIT:
        TRACE_INFO (USB_MSD_SBC, "SBC:ReqSense\n");
        sbc_state = SBC_STATE_WRITE;
        /* Fall through ...  */

    case SBC_STATE_WRITE:
        // Start transfer
        bStatus = usb_bot_write (&pLun->sRequestSenseData, pCommandState->dLength,
                                 pTransfer);

        if (bStatus != USB_BOT_STATUS_SUCCESS)
        {
            TRACE_ERROR (USB_MSD_SBC, "SBC:Write error\n");
            bResult = SBC_STATUS_ERROR;
        }
        else
        {
            sbc_state = SBC_STATE_WAIT_WRITE;
        }
        break;
    
    case SBC_STATE_WAIT_WRITE:
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;
            bResult = sbc_status (pTransfer->bStatus);
            pCommandState->dLength -= pTransfer->dBytesTransferred;
        }
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
 * \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
 * 
 */
static sbc_status_t
sbc_test_unit_ready (usb_msd_lun_t *pLun)
{
    msd_status_t status;

    TRACE_INFO (USB_MSD_SBC, "SBC:TstUnitRdy\n");
    
    status = lun_status_get (pLun);

    switch (status)
    {
    case MSD_STATUS_READY:
        TRACE_INFO (USB_MSD_SBC, "SBC:Ready\n");
        break;
        
    case MSD_STATUS_BUSY:
        TRACE_INFO (USB_MSD_SBC, "SBC:Busy\n");
        lun_update_sense_data (pLun, SBC_SENSE_KEY_NOT_READY, 0, 0);
        break;
        
    case MSD_STATUS_NODEVICE:
        TRACE_INFO (USB_MSD_SBC, "SBC:?\n");
        lun_update_sense_data (pLun, SBC_SENSE_KEY_NOT_READY,
                               SBC_ASC_MEDIUM_NOT_PRESENT, 0);
        break;
    }
    
    return SBC_STATUS_SUCCESS;
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
            TRACE_INFO (USB_MSD_SBC, "SBC:Bad page code 0X%02X\n",
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
sbc_status_t
sbc_process_command (S_usb_bot_command_state *pCommandState)
{
    sbc_status_t bResult = SBC_STATUS_INCOMPLETE;
    S_sbc_command *pCommand;
    usb_msd_cbw_t *pCbw = &pCommandState->sCbw;
    usb_msd_lun_t *pLun;

    /* Need to cast array containing command to union of command
       structures.  */
    pCommand = (S_sbc_command *) pCommandState->sCbw.pCommand;

    // If lun in error should we do bad parameter handling?
    pLun = lun_get (pCbw->bCBWLUN);
    if (!pLun)
        return SBC_STATUS_ERROR;

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
        TRACE_INFO (USB_MSD_SBC, "SBC:Verify (10)\n");
        // Nothing to do
        bResult = SBC_STATUS_SUCCESS;
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
        bResult = SBC_STATUS_SUCCESS;
        break;

    default:
        bResult = SBC_STATUS_PARAMETER;
    }

    switch (bResult)
    {
    case SBC_STATUS_PARAMETER:
        // Windows sends this
        TRACE_INFO (USB_MSD_SBC, "SBC:Bad command 0x%02X\n",
                    pCbw->pCommand[0]);

        lun_update_sense_data (pLun, SBC_SENSE_KEY_ILLEGAL_REQUEST,
                               SBC_ASC_INVALID_COMMAND_OPERATION_CODE,
                               /* SBC_ASC_INVALID_FIELD_IN_CDB, */
                               0);
        break;
    
    case SBC_STATUS_ERROR:
        TRACE_ERROR (USB_MSD_SBC, "SBC:Command failed\n");
        lun_update_sense_data (pLun, SBC_SENSE_KEY_MEDIUM_ERROR,
                               SBC_ASC_INVALID_FIELD_IN_CDB, 0);
        break;

    case SBC_STATUS_INCOMPLETE:
        lun_update_sense_data (pLun, SBC_SENSE_KEY_NO_SENSE, 0, 0);
        break;

    case SBC_STATUS_SUCCESS:
        TRACE_DEBUG (USB_MSD_SBC, "SBC:Command complete\n");
        lun_update_sense_data (pLun, SBC_SENSE_KEY_NO_SENSE, 0, 0);
        break;
    }

    /* If finished operation, reset state.  */
    if (bResult != SBC_STATUS_INCOMPLETE)
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

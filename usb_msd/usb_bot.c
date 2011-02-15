#include <stdlib.h>
#include "delay.h"
#include "usb_trace.h"
#include "usb_bot.h"

/* See Universal Serial Bus Mass Storage Class Bulk-Only Transport
   document. www.usb.org/developers/devclass_docs/usb_msc_overview_1.2.pdf

   See also http://www.atmel.com/dyn/resources/prod_documents/doc6283.pdf

   When severe errors occur during command or data transfers, the
   device must halt both Bulk endpoints and wait for a Reset Recovery
   procedure.  The Reset Recovery sequence goes as follows:  The host
   issues a Bulk-Only Mass Storage Reset request.  The host issues two
   CLEAR_FEATURE requests to unhalt each endpoint.  A device waiting for
   a Reset Recovery must not carry out CLEAR_FEATURE requests trying
   to unhalt either Bulk endpoint until after a Reset request has been
   received.  This enables the host to distinguish between severe and
   minor errors.  The only major error defined by the Bulk-Only
   Transport standard is when a CBW is not valid.  This means one or
   more of the following: The CBW is not received after a CSW has
   been sent or a reset.  The CBW is not exactly 31 bytes in length.
   The dCBWSignature field of the CBW is not equal to 43425355h.
*/


/** Set flag(s) in a register */
#define SET(register, flags)        ((register) = (register) | (flags))
/** Clear flag(s) in a register */
#define CLEAR(register, flags)      ((register) &= ~(flags))

/** Poll the status of flags in a register */
#define ISSET(register, flags)      (((register) & (flags)) == (flags))
/** Poll the status of flags in a register */
#define ISCLEARED(register, flags)  (((register) & (flags)) == 0)


//! \brief  Possible states of the MSD driver
typedef enum
{
    USB_BOT_STATE_INIT,
//! \brief BOT is waiting for USB to be enumerated
    USB_BOT_STATE_WAIT,
//! \brief  BOT is expecting a command block wrapper
    USB_BOT_STATE_READ_CBW,
//! \brief  BOT is waiting for the transfer to finish
    USB_BOT_STATE_WAIT_CBW,
//! \brief  BOT is processing the received command
    USB_BOT_STATE_PROCESS_CBW,
//! \brief  BOT is starting the transmission of a command status wrapper
    USB_BOT_STATE_SEND_CSW,
//! \brief  BOT is waiting for the CSW transmission to finish
    USB_BOT_STATE_WAIT_CSW
} usb_bot_state_t;


// Class-specific requests
//! Bulk transfer reset
#define MSD_BULK_ONLY_RESET                     0xFF
//! Get maximum number of LUNs
#define MSD_GET_MAX_LUN                         0xFE



//! \brief  MSD driver state variables
//! \see    S_usb_bot_command_state
//! \see    S_std_class
typedef struct
{
    //!< Maximum LUN index.  This is one less than the number of LUNs.
    uint8_t max_lun;
    //!< Current state of the driver
    usb_bot_state_t state;
    uint8_t wait_reset_recovery;
    usb_t usb;
    uint16_t errors[8];
} usb_bot_t;


static usb_bot_t usb_bot_dev;
static usb_bot_t *usb_bot = &usb_bot_dev;


uint16_t usb_bot_transfer_bytes (usb_bot_transfer_t *pTransfer)
{
    return pTransfer->transferred;
}


usb_bot_status_t usb_bot_transfer_status (usb_bot_transfer_t *pTransfer)
{
    switch (pTransfer->status)
    {
    case USB_STATUS_SUCCESS:
        return USB_BOT_STATUS_SUCCESS;

    case USB_STATUS_PENDING:
        return USB_BOT_STATUS_INCOMPLETE;
        
    case USB_STATUS_RESET:
        TRACE_ERROR (USB_BOT, "BOT:Endpoint reset\n");
        
    case USB_STATUS_BUSY:
    case USB_STATUS_ABORTED:
        break;
    }

    return pTransfer->write ? USB_BOT_STATUS_ERROR_USB_WRITE
        : USB_BOT_STATUS_ERROR_USB_READ;
}


static void
usb_bot_transfer_init (usb_bot_transfer_t *pTransfer, uint16_t size, bool write)
{
    pTransfer->status = USB_STATUS_PENDING;
    pTransfer->transferred = 0;
    pTransfer->write = write;
    // This is only used for debugging
    pTransfer->requested = size;
}


/**
 * This function is to be used as a callback for BOT transfers.
 * 
 */
static void
usb_bot_transfer_callback (void *arg, usb_transfer_t *usb_transfer)
{
    usb_bot_transfer_t *bot_transfer = arg;

    bot_transfer->status = usb_transfer->status;
    bot_transfer->transferred = usb_transfer->transferred;
}


usb_bot_status_t
usb_bot_write (const void *buffer, uint16_t size, usb_bot_transfer_t *pTransfer)
{
    usb_bot_status_t status;

    usb_bot_transfer_init (pTransfer, size, true);
    status = usb_write_async (usb_bot->usb, buffer, size,
                              usb_bot_transfer_callback,
                              pTransfer);

    if (status != USB_STATUS_SUCCESS)
    {
        pTransfer->status = status;
        return USB_BOT_STATUS_ERROR_USB_WRITE;
    }

    return  USB_BOT_STATUS_INCOMPLETE;
}


usb_bot_status_t
usb_bot_read (void *buffer, uint16_t size, usb_bot_transfer_t *pTransfer)
{
    usb_bot_status_t status;

    usb_bot_transfer_init (pTransfer, size, false);

    status = usb_read_async (usb_bot->usb, buffer, size, 
                             usb_bot_transfer_callback,
                             pTransfer);

    if (status != USB_STATUS_SUCCESS)
    {
        pTransfer->status = status;
        return USB_BOT_STATUS_ERROR_USB_READ;
    }

    return  USB_BOT_STATUS_INCOMPLETE;
}


/**
 * Returns the expected transfer length and direction (IN, OUT or don't
 * care) from the host point-of-view.
 * 
 * \param  pCbw    Pointer to the CBW to examinate
 * \param  pLength Expected length of command
 * \param  pType   Expected direction of command
 * 
 */
void
usb_bot_get_command_information (usb_msd_cbw_t *pCbw, uint32_t *pLength, 
                                 uint8_t *pType)
{
    // Expected host transfer direction and length
    *pLength = pCbw->dCBWDataTransferLength;

    if (*pLength == 0)
    {
        *pType = USB_BOT_NO_TRANSFER;
    }
    else if (ISSET (pCbw->bmCBWFlags, MSD_CBW_DEVICE_TO_HOST))
    {
        *pType = USB_BOT_DEVICE_TO_HOST;
    }
    else
    {
        *pType = USB_BOT_HOST_TO_DEVICE;
    }
}


/**
 * Post-processes a command given the case identified during the
 * pre-processing step.
 * 
 * Depending on the case, one of the following actions can be done:
 * - Bulk IN endpoint is stalled
 * - Bulk OUT endpoint is stalled
 * - CSW status set to phase error
 * 
 */
static void
usb_bot_post_process_command (S_usb_bot_command_state *pCommandState)
{
    usb_msd_csw_t *pCsw = &pCommandState->sCsw;

    pCsw->dCSWDataResidue += pCommandState->dLength;

    // Stall Bulk IN endpoint ?
    if (ISSET (pCommandState->bCase, USB_BOT_CASE_STALL_IN))
    {
        TRACE_INFO (USB_BOT, "BOT:StallIn\n");
        usb_halt (usb_bot->usb, USB_BOT_EPT_BULK_IN, 1);
    }

    // Stall Bulk OUT endpoint ?
    if (ISSET (pCommandState->bCase, USB_BOT_CASE_STALL_OUT))
    {
        TRACE_INFO (USB_BOT, "BOT:StallOut\n");
        usb_halt (usb_bot->usb, USB_BOT_EPT_BULK_OUT, 1);
    }

    // Set CSW status code to phase error ?
    if (ISSET (pCommandState->bCase, USB_BOT_CASE_PHASE_ERROR))
    {
        TRACE_INFO (USB_BOT, "BOT:PhaseErr\n");
        pCsw->bCSWStatus = MSD_CSW_PHASE_ERROR;
    }
}


/**
 * Handler for incoming SETUP requests on default control endpoint 0.
 * This runs as part of UDP interrupt so avoid tracing.
 */
static bool
usb_bot_request_handler (usb_t usb, usb_setup_t *setup)
{
    switch (setup->request)
    {
    // Intercept endpoint halt
    case USB_CLEAR_FEATURE:
        switch (setup->value)
        {
        case USB_ENDPOINT_HALT:
            // Do not clear the endpoint halt status if the device is waiting
            // for a reset recovery sequence
            if (usb_bot->wait_reset_recovery) 
            {
                TRACE_ERROR (USB_BOT, "BOT:reset wait\n");
                usb_control_write_zlp (usb);
                break;
            }
            // Fall through
        default:
            return 0;
        }
        break;

        // The MSD requests should be passed up to usb_msd
    case MSD_GET_MAX_LUN:
        TRACE_INFO (USB_BOT, "BOT:MaxLun %d\n", usb_bot->max_lun);
        if (setup->value == 0 && setup->index == 0 && setup->length == 1)
            usb_control_write (usb_bot->usb, &usb_bot->max_lun, 1);
        else
            usb_control_stall (usb);
        break;
        
    case MSD_BULK_ONLY_RESET:
        TRACE_INFO (USB_BOT, "BOT:Reset\n");
        if (setup->value == 0 && setup->index == 0 && setup->length == 0)
        {
            // Reset the MSD driver.  TODO, what else do we do?
            usb_bot->state = USB_BOT_STATE_WAIT;
            usb_bot->wait_reset_recovery = false;
            usb_control_write_zlp (usb);
        }
        else
        {
            usb_control_stall (usb);
        }
        break;
    
    default:
        // Forward request to standard handler
        return 0;
    }
    return 1;
}


/**
 * Initializes a BOT driver and the associated USB driver.
 * 
 * \param   bNumLun Number of LUN in list
 * 
 */
bool usb_bot_init (uint8_t num, const usb_dsc_t *descriptors)
{
    TRACE_INFO (USB_BOT, "BOT:Init\n");

    usb_bot->max_lun = num - 1;

    usb_bot->state = USB_BOT_STATE_INIT;
    usb_bot->usb = usb_init (descriptors, (void *)usb_bot_request_handler);
    
    return usb_bot->usb != 0;
}


void
usb_bot_abort (S_usb_bot_command_state *pCommandState)
{
    usb_msd_cbw_t *pCbw = &pCommandState->sCbw;

    /* This function is required for parameter errors.  */

    // Stall the endpoint waiting for data
    if (!ISSET (pCbw->bmCBWFlags, MSD_CBW_DEVICE_TO_HOST))
    {
        // Stall the OUT endpoint : host to device
        usb_halt (usb_bot->usb, USB_BOT_EPT_BULK_OUT, 1);
        TRACE_ERROR (USB_BOT, "BOT:StallOut 1\n");
    }
    else
    {
        // Stall the IN endpoint : device to host
        usb_halt (usb_bot->usb, USB_BOT_EPT_BULK_IN, 1);
        TRACE_ERROR (USB_BOT, "BOT:StallIn 1\n");
    }
}


static void
usb_bot_command_read_error (void)
{
    // Wait for a reset recovery
    usb_bot->wait_reset_recovery = true;
    
    // Halt the bulk in and bulk out pipes
    usb_halt (usb_bot->usb, USB_BOT_EPT_BULK_OUT, 1);
    usb_halt (usb_bot->usb, USB_BOT_EPT_BULK_IN, 1);
}


bool
usb_bot_command_read (S_usb_bot_command_state *pCommandState)
{
    usb_msd_cbw_t *pCbw = &pCommandState->sCbw;
    usb_msd_csw_t *pCsw = &pCommandState->sCsw;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;
    usb_bot_status_t bStatus;

    switch (usb_bot->state)
    {
    case USB_BOT_STATE_READ_CBW:
        TRACE_DEBUG (USB_BOT, "BOT:ReadCBW\n");
            
        usb_bot_read (pCbw, MSD_CBW_SIZE, pTransfer);
        usb_bot->state = USB_BOT_STATE_WAIT_CBW;
        return 0;
        break;

    case USB_BOT_STATE_WAIT_CBW:
        bStatus = usb_bot_transfer_status (pTransfer);
        if (bStatus == USB_BOT_STATUS_ERROR_USB_READ)
        {
            usb_bot_error_log (bStatus);
            TRACE_ERROR (USB_BOT, "BOT:ReadCBW fail\n");
            usb_bot->state = USB_BOT_STATE_READ_CBW;
        }
        else if (bStatus == USB_BOT_STATUS_SUCCESS)
        {
            // Copy the tag
            pCsw->dCSWTag = pCbw->dCBWTag;
            // Check that the CBW is 31 bytes long
            if (usb_bot_transfer_bytes (pTransfer) != MSD_CBW_SIZE)
            {
                usb_bot_error_log (USB_BOT_STATUS_ERROR_USB_READ);
                TRACE_ERROR (USB_BOT, "BOT:Invalid CBW size\n");
                
                usb_bot_command_read_error ();
                
                pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                usb_bot->state = USB_BOT_STATE_READ_CBW;
            }
            // Check the CBW Signature
            else if (pCbw->dCBWSignature != MSD_CBW_SIGNATURE)
            {
                usb_bot_error_log (USB_BOT_STATUS_ERROR_CBW_FORMAT);
                TRACE_ERROR (USB_BOT, "BOT:Invalid CBW sig\n0x%X\n",
                             (unsigned int)pCbw->dCBWSignature);
                
                usb_bot_command_read_error ();
                
                pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                usb_bot->state = USB_BOT_STATE_READ_CBW;
            }
            usb_bot->state = USB_BOT_STATE_PROCESS_CBW;
            return 1;
        }
        break;

    default:
        TRACE_ERROR (USB_BOT, "BOT:Bad state %d\n", usb_bot->state);
        break;
    }
    return 0;
}


bool
usb_bot_status_write (S_usb_bot_command_state *pCommandState)
{
    usb_msd_csw_t *pCsw = &pCommandState->sCsw;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;
    usb_bot_status_t bStatus;

    switch (usb_bot->state)
    {
    case USB_BOT_STATE_PROCESS_CBW:
        usb_bot_post_process_command (pCommandState);
        usb_bot->state = USB_BOT_STATE_SEND_CSW;
        break;

    case USB_BOT_STATE_SEND_CSW:
        // MPH hack.  We cannot send CSW while endpoint halted.
        if (usb_halt_p (usb_bot->usb, USB_BOT_EPT_BULK_IN))
            break;

        pCsw->dCSWSignature = MSD_CSW_SIGNATURE;

        TRACE_DEBUG (USB_BOT, "BOT:SendCSW\n");
    
        // Start the CSW write operation
        usb_bot_write (pCsw, MSD_CSW_SIZE, pTransfer);
        usb_bot->state = USB_BOT_STATE_WAIT_CSW;
        break;

    case USB_BOT_STATE_WAIT_CSW:
        bStatus = usb_bot_transfer_status (pTransfer);
        if (bStatus == USB_BOT_STATUS_ERROR_USB_WRITE)
        {
            usb_bot_error_log (bStatus);
            TRACE_ERROR (USB_BOT, "BOT:SendCSW fail\n");            
            usb_bot->state = USB_BOT_STATE_READ_CBW;
            return 1;
        }
        else if (bStatus == USB_BOT_STATUS_SUCCESS)
        {
            usb_bot->state = USB_BOT_STATE_READ_CBW;
            return 1;
        }
        break;

    default:
        TRACE_ERROR (USB_BOT, "BOT:Bad state %d\n", usb_bot->state);
        break;
    }
    return 0;
}


bool
usb_bot_ready_p (void)
{
    if (!usb_bot->usb)
        return 0;

    usb_poll (usb_bot->usb);

    switch (usb_bot->state)
    {
    case USB_BOT_STATE_INIT:
        if (usb_awake_p (usb_bot->usb)
            || usb_configured_p (usb_bot->usb))
            usb_bot->state = USB_BOT_STATE_WAIT;
        break;

    case USB_BOT_STATE_WAIT:
        if (usb_configured_p (usb_bot->usb))
        {
            TRACE_INFO (USB_BOT, "BOT:Connected\n");

            // Some folks reckon a delay is needed for Windows
            delay_ms (100);

            usb_bot->state = USB_BOT_STATE_READ_CBW;
            usb_bot->wait_reset_recovery = false;
            return 1;
        }
        break;
            
    default:
        if (usb_configured_p (usb_bot->usb))
            return 1;

        TRACE_INFO (USB_BOT, "BOT:Disconnected\n");
        usb_bot->state = USB_BOT_STATE_INIT;
        break;
    }
    return 0;
}


void 
usb_bot_error_log (usb_bot_status_t status)
{
    unsigned int index;

    index = status - USB_BOT_STATUS_ERROR_CBW_PARAMETER;

    if (index < ARRAY_SIZE (usb_bot_dev.errors))
        usb_bot_dev.errors[index]++;
}

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
} usb_bot_t;


static usb_bot_t bot_dev;
static usb_bot_t *bot = &bot_dev;


static usb_bot_status_t
usb_bot_status (usb_status_t status)
{
    switch (status)
    {
    case USB_STATUS_SUCCESS:
        return USB_BOT_STATUS_SUCCESS;

    case USB_STATUS_RESET:
        TRACE_ERROR (USB_BOT, "BOT:Endpoint reset\n");

    default:
        return USB_BOT_STATUS_ERROR;
    }
}


/**
 * This function is to be used as a callback for BOT transfers.
 * 
 */
static void
usb_bot_callback (void *arg, usb_transfer_t *usb_transfer)
{
    usb_bot_transfer_t *bot_transfer = arg;

    bot_transfer->bSemaphore++;
    bot_transfer->bStatus = usb_bot_status (usb_transfer->status);
    bot_transfer->dBytesTransferred = usb_transfer->transferred;
    bot_transfer->dBytesRemaining = usb_transfer->remaining;
}


usb_bot_status_t
usb_bot_write (const void *buffer, uint16_t size, void *pTransfer)
{
    return usb_bot_status (usb_write_async (bot->usb, buffer, size,
                                            usb_bot_callback,
                                            pTransfer));
}


usb_bot_status_t
usb_bot_read (void *buffer, uint16_t size, void *pTransfer)
{
    return usb_bot_status (usb_read_async (bot->usb, buffer, size, 
                                           usb_bot_callback,
                                           pTransfer));
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
        usb_halt (bot->usb, USB_BOT_EPT_BULK_IN, 1);
    }

    // Stall Bulk OUT endpoint ?
    if (ISSET (pCommandState->bCase, USB_BOT_CASE_STALL_OUT))
    {
        TRACE_INFO (USB_BOT, "BOT:StallOut\n");
        usb_halt (bot->usb, USB_BOT_EPT_BULK_OUT, 1);
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
            if (bot->wait_reset_recovery) 
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
        TRACE_INFO (USB_BOT, "BOT:MaxLun %d\n", bot->max_lun);
        if (setup->value == 0 && setup->index == 0 && setup->length == 1)
            usb_control_write (bot->usb, &bot->max_lun, 1);
        else
            usb_control_stall (usb);
        break;
        
    case MSD_BULK_ONLY_RESET:
        TRACE_INFO (USB_BOT, "BOT:Reset\n");
        if (setup->value == 0 && setup->index == 0 && setup->length == 0)
        {
            // Reset the MSD driver.  TODO, what else do we do?
            bot->state = USB_BOT_STATE_WAIT;
            bot->wait_reset_recovery = false;
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

    bot->max_lun = num - 1;

    bot->state = USB_BOT_STATE_INIT;
    bot->usb = usb_init (descriptors, (void *)usb_bot_request_handler);
    
    return bot->usb != 0;
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
        usb_halt (bot->usb, USB_BOT_EPT_BULK_OUT, 1);
        TRACE_ERROR (USB_BOT, "BOT:StallOut 1\n");
    }
    else
    {
        // Stall the IN endpoint : device to host
        usb_halt (bot->usb, USB_BOT_EPT_BULK_IN, 1);
        TRACE_ERROR (USB_BOT, "BOT:StallIn 1\n");
    }
}


static void
bot_error (void)
{
    
    // Wait for a reset recovery
    bot->wait_reset_recovery = true;
    
    // Halt the bulk in and bulk out pipes
    usb_halt (bot->usb, USB_BOT_EPT_BULK_OUT, 1);
    usb_halt (bot->usb, USB_BOT_EPT_BULK_IN, 1);
}


bool
usb_bot_command_read (S_usb_bot_command_state *pCommandState)
{
    usb_msd_cbw_t *pCbw = &pCommandState->sCbw;
    usb_msd_csw_t *pCsw = &pCommandState->sCsw;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;
    usb_bot_status_t bStatus;

    switch (bot->state)
    {
    case USB_BOT_STATE_READ_CBW:
        TRACE_DEBUG (USB_BOT, "BOT:ReadCBW\n");
            
        pTransfer->bSemaphore = 0;
        bStatus = usb_bot_read (pCbw, MSD_CBW_SIZE, pTransfer);

        if (bStatus == USB_BOT_STATUS_SUCCESS)
            bot->state = USB_BOT_STATE_WAIT_CBW;
        else
            TRACE_ERROR (USB_BOT, "BOT:ReadCBW fail\n");            
        return 0;
        break;

    case USB_BOT_STATE_WAIT_CBW:
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;
    
            if (pTransfer->bStatus == USB_BOT_STATUS_SUCCESS)
            {
                // Copy the tag
                pCsw->dCSWTag = pCbw->dCBWTag;
                // Check that the CBW is 31 bytes long
                if ((pTransfer->dBytesTransferred != MSD_CBW_SIZE)
                    || (pTransfer->dBytesRemaining != 0))
                {
                    TRACE_ERROR (USB_BOT, "BOT:Invalid CBW size\n");

                    bot_error ();
                    
                    pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                    bot->state = USB_BOT_STATE_READ_CBW;
                }
                // Check the CBW Signature
                else if (pCbw->dCBWSignature != MSD_CBW_SIGNATURE)
                {
                    TRACE_ERROR (USB_BOT, "BOT:Invalid CBW sig\n0x%X\n",
                                (unsigned int)pCbw->dCBWSignature);
                    
                    bot_error ();
                    
                    pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                    bot->state = USB_BOT_STATE_READ_CBW;
                }
                bot->state = USB_BOT_STATE_PROCESS_CBW;
                return 1;
            }
            else
            {
                bot->state = USB_BOT_STATE_READ_CBW;
            }
        }
        break;

    default:
        TRACE_ERROR (USB_BOT, "BOT:Bad state %d\n", bot->state);
        break;
    }
    return 0;
}


bool
usb_bot_status_set (S_usb_bot_command_state *pCommandState)
{
    usb_msd_csw_t *pCsw = &pCommandState->sCsw;
    usb_bot_transfer_t *pTransfer = &pCommandState->sTransfer;
    usb_bot_status_t bStatus;

    switch (bot->state)
    {
    case USB_BOT_STATE_PROCESS_CBW:
        usb_bot_post_process_command (pCommandState);
        bot->state = USB_BOT_STATE_SEND_CSW;
        break;

    case USB_BOT_STATE_SEND_CSW:
        // MPH hack.  We cannot send CSW while endpoint halted.
        if (usb_halt_p (bot->usb, USB_BOT_EPT_BULK_IN))
            break;

        pCsw->dCSWSignature = MSD_CSW_SIGNATURE;

        TRACE_DEBUG (USB_BOT, "BOT:SendCSW\n");
    
        // Start the CSW write operation
        pTransfer->bSemaphore = 0;
        bStatus = usb_bot_write (pCsw, MSD_CSW_SIZE, pTransfer);
    
        if (bStatus == USB_BOT_STATUS_SUCCESS)
            bot->state = USB_BOT_STATE_WAIT_CSW;
        else
            TRACE_ERROR (USB_BOT, "BOT:SendCSW fail\n");            
        break;

    case USB_BOT_STATE_WAIT_CSW:
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;
            bot->state = USB_BOT_STATE_READ_CBW;
            return 1;
        }
        break;

    default:
        TRACE_ERROR (USB_BOT, "BOT:Bad state %d\n", bot->state);
        break;
    }
    return 0;
}


bool
usb_bot_ready_p (void)
{
    usb_poll (bot->usb);

    switch (bot->state)
    {
    case USB_BOT_STATE_INIT:
        if (usb_awake_p (bot->usb))
            bot->state = USB_BOT_STATE_WAIT;
        break;

    case USB_BOT_STATE_WAIT:
        if (usb_configured_p (bot->usb))
        {
            TRACE_INFO (USB_BOT, "BOT:Connected\n");

            // Some folks reckon a delay is needed for Windows
            delay_ms (100);

            bot->state = USB_BOT_STATE_READ_CBW;
            bot->wait_reset_recovery = false;
            return 1;
        }
        break;
            
    default:
        if (usb_configured_p (bot->usb))
            return 1;

        TRACE_INFO (USB_BOT, "BOT:Disconnected\n");
        bot->state = USB_BOT_STATE_INIT;
        break;
    }
    return 0;
}

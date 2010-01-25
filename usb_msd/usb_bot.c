#include <stdlib.h>
#include "delay.h"
#include "usb_trace.h"
#include "usb_bot.h"

/* See Universal Serial Bus Mass Storage Class Bulk-Only Transport
   document. www.usb.org/developers/devclass_docs/usb_msc_overview_1.2.pdf
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
    uint8_t max_lun;             //!< Maximum LUN index
    usb_bot_state_t state;       //!< Current state of the driver
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
 * A S_usb_bot_transfer structure is updated with the method results.
 * \param   pTransfer         Pointer to USB transfer result structure 
 * \param   bStatus           Operation result code
 * \param   dBytesTransferred Number of bytes transferred by the command
 * \param   dBytesRemaining   Number of bytes not transferred
 * 
 */
static void
usb_bot_callback (S_usb_bot_transfer *pTransfer,
                  uint8_t bStatus,
                  uint32_t dBytesTransferred,
                  uint32_t dBytesRemaining)
{
    pTransfer->bSemaphore++;
    pTransfer->bStatus = usb_bot_status (bStatus);
    pTransfer->dBytesTransferred = dBytesTransferred;
    pTransfer->dBytesRemaining = dBytesRemaining;
}


usb_bot_status_t
usb_bot_write (const void *buffer, uint16_t size, void *pTransfer)
{
    return usb_bot_status (usb_write_async (bot->usb, buffer, size,
                                            (udp_callback_t) usb_bot_callback,
                                            pTransfer));
}


usb_bot_status_t
usb_bot_read (void *buffer, uint16_t size, void *pTransfer)
{
    return usb_bot_status (usb_read_async (bot->usb, buffer, size, 
                                           (udp_callback_t) usb_bot_callback,
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
        usb_halt (bot->usb, USB_BOT_EPT_BULK_IN, USB_SET_FEATURE);
    }

    // Stall Bulk OUT endpoint ?
    if (ISSET (pCommandState->bCase, USB_BOT_CASE_STALL_OUT))
    {
        TRACE_INFO (USB_BOT, "BOT:StallOut\n");
        usb_halt (bot->usb, USB_BOT_EPT_BULK_OUT, USB_SET_FEATURE);
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
usb_bot_request_handler (usb_t usb, udp_setup_t *setup)
{
    // Handle requests
    switch (setup->request)
    {
    case USB_CLEAR_FEATURE:
        switch (setup->value)
        {
        case USB_ENDPOINT_HALT:
            TRACE_ERROR (USB_BOT, "BOT:Halt %d\n", bot->wait_reset_recovery);
        
            // Do not clear the endpoint halt status if the device is waiting
            // for a reset recovery sequence
            if (!bot->wait_reset_recovery) 
                return 0;

            usb_control_write_zlp (usb);
            break;
        
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
            // Reset the MSD driver.  TODO, what do we do?
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
        usb_halt (bot->usb,USB_BOT_EPT_BULK_OUT, USB_SET_FEATURE);
        TRACE_ERROR (USB_BOT, "BOT:StallOut 1\n");
    }
    else
    {
        // Stall the IN endpoint : device to host
        usb_halt (bot->usb,USB_BOT_EPT_BULK_IN, USB_SET_FEATURE);
        TRACE_ERROR (USB_BOT, "BOT:StallIn 1\n");
    }
}


bool
usb_bot_command_get (S_usb_bot_command_state *pCommandState)
{
    usb_msd_cbw_t *pCbw = &pCommandState->sCbw;
    usb_msd_csw_t *pCsw = &pCommandState->sCsw;
    S_usb_bot_transfer *pTransfer = &pCommandState->sTransfer;
    usb_bot_status_t bStatus;

    switch (bot->state)
    {
    case USB_BOT_STATE_READ_CBW:
        TRACE_DEBUG (USB_BOT, "BOT:ReadCBW\n");
            
        pTransfer->bSemaphore = 0;
        bStatus = usb_bot_read (pCbw, MSD_CBW_SIZE, pTransfer);

        if (bStatus == USB_BOT_STATUS_SUCCESS)
            bot->state = USB_BOT_STATE_WAIT_CBW;
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
                    
                    // Wait for a reset recovery
                    bot->wait_reset_recovery = true;
                    
                    // Halt the bulk in and bulk out pipes
                    usb_halt (bot->usb, USB_BOT_EPT_BULK_OUT, USB_SET_FEATURE);
                    usb_halt (bot->usb, USB_BOT_EPT_BULK_IN, USB_SET_FEATURE);
                    
                    pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                    bot->state = USB_BOT_STATE_READ_CBW;
                }
                // Check the CBW Signature
                else if (pCbw->dCBWSignature != MSD_CBW_SIGNATURE)
                {
                    TRACE_ERROR (USB_BOT, "BOT:Invalid CBW sig\n0x%X\n",
                                (unsigned int)pCbw->dCBWSignature);
                    
                    // Wait for a reset recovery
                    bot->wait_reset_recovery = true;
                    
                    // Halt the bulk in and bulk out pipes
                    usb_halt (bot->usb, USB_BOT_EPT_BULK_OUT, USB_SET_FEATURE);
                    usb_halt (bot->usb, USB_BOT_EPT_BULK_IN, USB_SET_FEATURE);
                    
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
    S_usb_bot_transfer *pTransfer = &pCommandState->sTransfer;
    usb_bot_status_t bStatus;

    switch (bot->state)
    {
    case USB_BOT_STATE_PROCESS_CBW:
        usb_bot_post_process_command (pCommandState);
        bot->state = USB_BOT_STATE_SEND_CSW;
        break;

    case USB_BOT_STATE_SEND_CSW:
        pCsw->dCSWSignature = MSD_CSW_SIGNATURE;

        TRACE_DEBUG (USB_BOT, "BOT:SendCSW\n");
    
        // Start the CSW write operation
        pTransfer->bSemaphore = 0;
        bStatus = usb_bot_write (pCsw, MSD_CSW_SIZE, pTransfer);
    
        if (bStatus == USB_BOT_STATUS_SUCCESS)
            bot->state = USB_BOT_STATE_WAIT_CSW;
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
            
    default:
        if (usb_configured_p (bot->usb))
            return 1;
        TRACE_INFO (USB_BOT, "BOT:Disconnected\n");
        break;
    }
    return 0;
}

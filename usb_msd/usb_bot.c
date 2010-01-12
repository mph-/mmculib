/**
 * \file usb_bot.c
 * Bulk only transfer 
 * 
 * Bulk only transfer handling functions
 *
 * AT91SAM7S-128 USB Mass Storage Device with SD Card by Michael Wolf\n
 * Copyright (C) 2008 Michael Wolf\n\n
 * 
 * This program is free software: you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation, either version 3 of the License, or\n
 * any later version.\n\n
 * 
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n
 * GNU General Public License for more details.\n\n
 * 
 * You should have received a copy of the GNU General Public License\n
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.\n
 *  
 */
#include <stdlib.h>
#include "usb_trace.h"
#include "usb_drv.h"
#include "usb_std.h"
#include "usb_bot.h"


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
//! \brief  Driver is expecting a command block wrapper
    USB_BOT_STATE_READ_CBW,
//! \brief  Driver is waiting for the transfer to finish
    USB_BOT_STATE_WAIT_CBW,
//! \brief  Driver is processing the received command
    USB_BOT_STATE_PROCESS_CBW,
//! \brief  Driver is starting the transmission of a command status wrapper
    USB_BOT_STATE_SEND_CSW,
//! \brief  Driver is waiting for the CSW transmission to finish
    USB_BOT_STATE_WAIT_CSW
} usb_bot_state_t;


//! \brief  MSD driver state variables
//! \see    S_usb_bot_command_state
//! \see    S_std_class
typedef struct
{
    uint8_t bMaxLun;             //!< Maximum LUN index
    usb_bot_state_t bState;      //!< Current state of the driver
    uint8_t isWaitResetRecovery; //!< Indicates if the driver is
} S_bot;

static S_bot usb_bot_struct;   //!< Structure holding MSD driver state variables
static S_bot *pBot = &usb_bot_struct;    //!< Pointer to Bulk-Only Transport (BOT) driver instance


bool
usb_bot_awake_p (void)
{
    return usb_awake_p ();
}


static usb_bot_status_t
usb_bot_status (usb_status_t status)
{
    switch (status)
    {
    case USB_STATUS_SUCCESS:
        return USB_BOT_STATUS_SUCCESS;

    case USB_STATUS_RESET:
        TRACE_INFO (USB_BOT, "BOT:Endpoint reset\n");

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
    return usb_bot_status (usb_write (USB_BOT_EPT_BULK_IN,
                                      buffer, size, (usb_callback_t) usb_bot_callback,
                                      pTransfer));
}


usb_bot_status_t
usb_bot_read (void *buffer, uint16_t size, void *pTransfer)
{
    return usb_bot_status (usb_read (USB_BOT_EPT_BULK_OUT,
                                     buffer, size, (usb_callback_t) usb_bot_callback,
                                     pTransfer));
}


/**
 * Resets the state of the BOT driver
 * 
 */
void
usb_bot_reset (void)
{
    TRACE_INFO (USB_BOT, "BOT:Reset\n");

    pBot->bState = USB_BOT_STATE_READ_CBW;
    pBot->isWaitResetRecovery = false;
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
usb_bot_get_command_information (S_msd_cbw *pCbw, uint32_t *pLength, 
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
    S_msd_csw *pCsw = &pCommandState->sCsw;

    pCsw->dCSWDataResidue += pCommandState->dLength;

    // Stall Bulk IN endpoint ?
    if (ISSET (pCommandState->bCase, USB_BOT_CASE_STALL_IN))
    {
        TRACE_INFO (USB_BOT, "BOT:StallIn\n");
        usb_halt (USB_BOT_EPT_BULK_IN, USB_SET_FEATURE);
    }

    // Stall Bulk OUT endpoint ?
    if (ISSET (pCommandState->bCase, USB_BOT_CASE_STALL_OUT))
    {
        TRACE_INFO (USB_BOT, "BOT:StallOut\n");
        usb_halt (USB_BOT_EPT_BULK_OUT, USB_SET_FEATURE);
    }

    // Set CSW status code to phase error ?
    if (ISSET (pCommandState->bCase, USB_BOT_CASE_PHASE_ERROR))
    {
        TRACE_INFO (USB_BOT, "BOT:PhaseErr\n");
        pCsw->bCSWStatus = MSD_CSW_PHASE_ERROR;
    }
}


/**
 * Handler for incoming SETUP requests on default Control endpoint 0.
 * This runs as part of USB interrupt so avoid tracing.
 */
static bool
usb_bot_request_handler (s_usb_request *pSetup)
{
    // Handle requests
    switch (pSetup->bRequest)
    {
    case USB_CLEAR_FEATURE:
//        TRACE_INFO (USB_BOT, "BOT:ClrFeat\n");
    
        switch (pSetup->wValue)
        {
        case USB_ENDPOINT_HALT:
//            TRACE_INFO (USB_BOT, "BOT:Halt\n");
        
            // Do not clear the endpoint halt status if the device is waiting
            // for a reset recovery sequence
            if (!pBot->isWaitResetRecovery) 
            {
                return 0;
            }
            else 
            {
//                TRACE_INFO (USB_BOT, "BOT:No\n");
            }
            usb_send_zlp0 (0,0);
            break;
        
        default:
            return 0;
        }
        break;
    
    case MSD_GET_MAX_LUN:
//        TRACE_INFO (USB_BOT, "BOT:MaxLun %d\n", pBot->bMaxLun);
    
        // Check request parameters
        if ((pSetup->wValue == false)
            && (pSetup->wIndex == false)
            && (pSetup->wLength == true))
        {
            usb_write (0, &pBot->bMaxLun, 1, 0, 0);
        }
        else
        {
            usb_stall (0);
        }
        break;
        
    case MSD_BULK_ONLY_RESET:
//        TRACE_INFO (USB_BOT, "BOT:Reset\n");
        
        // Check parameters
        if ((pSetup->wValue == 0)
            && (pSetup->wIndex == 0)
            && (pSetup->wLength == 0)) {
            
            // Reset the MSD driver
            usb_bot_reset ();
            usb_send_zlp0 (0,0);
        }
        else
        {
            usb_stall (0);
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
bool usb_bot_init (uint8_t num)
{
    TRACE_INFO (USB_BOT, "BOT:Init\n");

    // LUNs
    pBot->bMaxLun = num - 1;

    usb_handler_register (usb_bot_request_handler);

    // Reset BOT driver
    usb_bot_reset ();

    /* FIXME, add timeout.  */
    while (!usb_configured_p ())
        continue;

    return 1;
}


void
usb_bot_abort (S_usb_bot_command_state *pCommandState)
{
    S_msd_cbw *pCbw = &pCommandState->sCbw;

    /* This is required for parameter error.  */
    // Stall the endpoint waiting for data
    if (!ISSET (pCbw->bmCBWFlags, MSD_CBW_DEVICE_TO_HOST))
    {
        // Stall the OUT endpoint : host to device
        usb_halt (USB_BOT_EPT_BULK_OUT, USB_SET_FEATURE);
        TRACE_ERROR (USB_BOT, "BOT:StallOut 1\n");
    }
    else
    {
        // Stall the IN endpoint : device to host
        usb_halt (USB_BOT_EPT_BULK_IN, USB_SET_FEATURE);
        TRACE_ERROR (USB_BOT, "BOT:StallIn 1\n");
    }
}


bool
usb_bot_command_get (S_usb_bot_command_state *pCommandState)
{
    S_msd_cbw *pCbw = &pCommandState->sCbw;
    S_msd_csw *pCsw = &pCommandState->sCsw;
    S_usb_bot_transfer *pTransfer = &pCommandState->sTransfer;
    usb_bot_status_t bStatus;

    switch (pBot->bState)
    {
    case USB_BOT_STATE_READ_CBW:
        TRACE_DEBUG (USB_BOT, "BOT:ReadCBW\n");
            
        pTransfer->bSemaphore = 0;
        bStatus = usb_bot_read (pCbw, MSD_CBW_SIZE, pTransfer);

        if (bStatus == USB_BOT_STATUS_SUCCESS)
            pBot->bState = USB_BOT_STATE_WAIT_CBW;
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
                    TRACE_INFO (USB_BOT, "BOT:Invalid CBW size\n");
                    
                    // Wait for a reset recovery
                    pBot->isWaitResetRecovery = true;
                    
                    // Halt the Bulk-IN and Bulk-OUT pipes
                    usb_halt (USB_BOT_EPT_BULK_OUT, USB_SET_FEATURE);
                    usb_halt (USB_BOT_EPT_BULK_IN, USB_SET_FEATURE);
                    
                    pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                    pBot->bState = USB_BOT_STATE_READ_CBW;
                }
                // Check the CBW Signature
                else if (pCbw->dCBWSignature != MSD_CBW_SIGNATURE)
                {
                    TRACE_INFO (USB_BOT, "BOT:Invalid CBW sig\n0x%X\n",
                                (unsigned int)pCbw->dCBWSignature);
                    
                    // Wait for a reset recovery
                    pBot->isWaitResetRecovery = true;
                    
                    // Halt the Bulk-IN and Bulk-OUT pipes
                    usb_halt (USB_BOT_EPT_BULK_OUT, USB_SET_FEATURE);
                    usb_halt (USB_BOT_EPT_BULK_IN, USB_SET_FEATURE);
                    
                    pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                    pBot->bState = USB_BOT_STATE_READ_CBW;
                }
                pBot->bState = USB_BOT_STATE_PROCESS_CBW;
                return 1;
            }
            else
            {
                pBot->bState = USB_BOT_STATE_READ_CBW;
            }
        }
        break;

    default:
        TRACE_ERROR (USB_BOT, "BOT:Bad state %d\n", pBot->bState);
        break;
    }
    return 0;
}


bool
usb_bot_status_set (S_usb_bot_command_state *pCommandState)
{
    S_msd_csw *pCsw = &pCommandState->sCsw;
    S_usb_bot_transfer *pTransfer = &pCommandState->sTransfer;
    usb_bot_status_t bStatus;

    switch (pBot->bState)
    {
    case USB_BOT_STATE_PROCESS_CBW:
        usb_bot_post_process_command (pCommandState);
        pBot->bState = USB_BOT_STATE_SEND_CSW;
        break;

    case USB_BOT_STATE_SEND_CSW:
        pCsw->dCSWSignature = MSD_CSW_SIGNATURE;

        TRACE_DEBUG (USB_BOT, "BOT:SendCSW\n");
    
        // Start the CSW write operation
        pTransfer->bSemaphore = 0;
        bStatus = usb_bot_write (pCsw, MSD_CSW_SIZE, pTransfer);
    
        if (bStatus == USB_BOT_STATUS_SUCCESS)
            pBot->bState = USB_BOT_STATE_WAIT_CSW;
        break;

    case USB_BOT_STATE_WAIT_CSW:
        if (pTransfer->bSemaphore > 0)
        {
            pTransfer->bSemaphore--;
            pBot->bState = USB_BOT_STATE_READ_CBW;
            return 1;
        }
        break;

    default:
        TRACE_ERROR (USB_BOT, "BOT:Bad state %d\n", pBot->bState);
        break;
    }
    return 0;
}

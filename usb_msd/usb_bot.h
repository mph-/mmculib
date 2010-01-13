/**
 * \file usb_bot.h
 * Header: Bulk only tranfer 
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
#ifndef USB_BOT_H_
#define USB_BOT_H_

#include "config.h"
#include "usb_msd_defs.h"
#include "usb.h"

//! \brief  Address of Bulk OUT endpoint
#define USB_BOT_EPT_BULK_OUT    UDP_EP_OUT

//! \brief  Address of Bulk IN endpoint
#define USB_BOT_EPT_BULK_IN     UDP_EP_IN

#define USB_BOT_IN_EP_SIZE      UDP_EP_OUT_SIZE
#define USB_BOT_OUT_EP_SIZE     UDP_EP_IN_SIZE


typedef enum
{
//! \brief  Method was successful
    USB_BOT_STATUS_SUCCESS = 0x00,
//! \brief  There was an error when trying to perform a method
    USB_BOT_STATUS_ERROR = 0x01,
} usb_bot_status_t;


//! \brief  Actions to perform during the post-processing phase of a command
//! \brief  Indicates that the CSW should report a phase error
#define USB_BOT_CASE_PHASE_ERROR            (1 << 0)

//! \brief  The driver should halt the Bulk IN pipe after the transfer
#define USB_BOT_CASE_STALL_IN               (1 << 1)

//! \brief  The driver should halt the Bulk OUT pipe after the transfer
#define USB_BOT_CASE_STALL_OUT              (1 << 2)

//! \name Possible direction values for a data transfer
//@{
#define USB_BOT_DEVICE_TO_HOST              0
#define USB_BOT_HOST_TO_DEVICE              1
#define USB_BOT_NO_TRANSFER                 2
//@}

//! \brief  Structure for holding the result of a USB transfer
//! \see    MSD_Callback
typedef struct
{
    uint32_t  dBytesTransferred; //!< Number of bytes transferred
    uint32_t  dBytesRemaining;   //!< Number of bytes not transferred
    unsigned char bSemaphore;    //!< Semaphore to indicate transfer completion
    unsigned char bStatus;       //!< Operation result code
} S_usb_bot_transfer;


//! \brief  Status of an executing command
//! \see    S_msd_cbw
//! \see    S_msd_csw
//! \see    S_usb_bot_transfer
typedef struct 
{
    S_usb_bot_transfer sTransfer;   //!< Current transfer status
    S_msd_cbw sCbw;             //!< Received CBW
    S_msd_csw sCsw;             //!< CSW to send
    uint8_t bCase;    	        //!< Actions to perform when command is complete
    uint32_t dLength;           //!< Remaining length of command
} S_usb_bot_command_state;


bool usb_bot_awake_p (void);

bool usb_bot_update (void);

bool usb_bot_init (uint8_t num, const usb_descriptors_t *descriptors);

usb_bot_status_t
usb_bot_write (const void *buffer, uint16_t size, void *pTransfer);

usb_bot_status_t
usb_bot_read (void *buffer, uint16_t size, void *pTransfer);

bool
usb_bot_command_get (S_usb_bot_command_state *pCommandState);

bool
usb_bot_status_set (S_usb_bot_command_state *pCommandState);

void
usb_bot_get_command_information (S_msd_cbw *pCbw, uint32_t *pLength, 
                                 uint8_t *pType);

void
usb_bot_abort (S_usb_bot_command_state *pCommandState);

#endif /*USB_BOT_H_*/

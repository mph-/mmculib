/**
 * \file usb_drv.h
 * Header: USB driver 
 * 
 * USB driver functions
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
#ifndef USB_DRV_H_
#define USB_DRV_H_

#include "config.h"
#include "usb_std.h"

typedef struct usb_struct
{
    bool configured;
} usb_dev_t;

typedef usb_dev_t *usb_t;


/* Highspeed device is NOT supported on AT91SAM7-128/256 */
#ifdef USB_HIGHSPEED
#undef USB_HIGHSPEED
#endif


#define NUM_OF_ENDPOINTS     3  //!< Number of used endpoint, including EP0

/**
 * \name Values returned by the API methods
 *  
 */
typedef enum
{
//! Last method has completed successfully
    USB_STATUS_SUCCESS = 0,
//! Method was aborted because the recipient (device, endpoint, ...) was busy
    USB_STATUS_LOCKED = 1,
//! Method was aborted because of abnormal status
    USB_STATUS_ABORTED = 2,
//! Method was aborted because the endpoint or the device has been reset
    USB_STATUS_RESET = 3,
//! Method status unknow
 USB_STATUS_UNKOWN = 4
} usb_status_t;


/**
 * \name Constant values used to track which USB state the device is currently in.
 * 
 */
//@{
//! Detached state
#define USB_STATE_DETACHED                          (1 << 0)
//! Attached state
#define USB_STATE_ATTACHED                          (1 << 1)
//! Powered state
#define USB_STATE_POWERED                           (1 << 2)
//! Default state
#define USB_STATE_DEFAULT                           (1 << 3)
//! Address state
#define USB_STATE_ADDRESS                           (1 << 4)
//! Configured state
#define USB_STATE_CONFIGURED                        (1 << 5)
//! Suspended state
#define USB_STATE_SUSPENDED                         (1 << 6)
//@}



/** 
 * \name UDP endpoint configuration
 * 
*/
//@{
#define EP0                 0   //!< Endpoint 0
#define EP0_BUFF_SIZE       8   //!< Endpoint 0 size
#define EP1                 1   //!< Endpoint 1
#define EP1_BUFF_SIZE       64  //!< Endpoint 1 size
#define EP2     2               //!< Endpoint 2
#define EP2_BUFF_SIZE       64  //!< Endpoint 2 size
#define EP3     3               //!< Endpoint 3
#define EP3_BUFF_SIZE       64  //!< Endpoint 3 size
//@}


typedef void (*usb_callback_t) (unsigned int, unsigned int,
                                unsigned int, unsigned int);


typedef bool (*usb_handler_t) (s_usb_request *);


/**
 * Structure for endpoint transfer parameters
 * 
 */
typedef struct
{
    // Transfer descriptor
    char                    *pData;             //!< \brief Transfer descriptor
                                                //!< pointer to a buffer where
                                                //!< the data is read/stored
    unsigned int            dBytesRemaining;    //!< \brief Number of remaining
                                                //!< bytes to transfer
    unsigned int            dBytesBuffered;     //!< \brief Number of bytes
                                                //!< which have been buffered
                                                //!< but not yet transferred
    unsigned int            dBytesTransferred;  //!< \brief Number of bytes
                                                //!< transferred for the current
                                                //!< operation
    usb_callback_t          fCallback;          //!< \brief Callback to invoke
                                                //!< after the current transfer
                                                //!< is complete
    void                    *pArgument;         //!< \brief Argument to pass to
                                                //!< the callback function                                                
    // Hardware information
    unsigned int            wMaxPacketSize;     //!< \brief Maximum packet size
                                                //!< for this endpoint
    unsigned int            dFlag;              //!< \brief Hardware flag to
                                                //!< clear upon data reception
    unsigned char           dNumFIFO;           //!< \brief Number of FIFO
                                                //!< buffers defined for this
                                                //!< endpoint
    unsigned int   dState;                      //!< Endpoint internal state
} __packed__ s_usb_endpoint;


extern volatile unsigned char usb_device_state; //!< Device status, connected/disconnected etc.

extern volatile unsigned char usb_configuration; 

bool usb_configured_p (void);

bool usb_awake_p (void);

unsigned char usb_check_bus_status (void);

void usb_soft_disable_device (void);

void usb_bus_reset_handler (void);

void usb_endpoint_handler (unsigned char endpoint);

void usb_stall (unsigned char endpoint);

bool usb_halt (unsigned char endpoint, unsigned char request);

usb_status_t usb_write (unsigned char endpoint,
                        const void *pData,
                        unsigned int len,
                        usb_callback_t fCallback,
                        void *pArgument);

usb_status_t usb_read (unsigned char endpoint,
                       void *pData,
                       unsigned int len,
                       usb_callback_t fCallback,
                       void *pArgument);

inline char usb_send_zlp0 (usb_callback_t fCallback, void *pArgument);

void usb_set_address (void);

void usb_configure_endpoint (unsigned char endpoint);

void usb_set_configuration (void);

usb_t usb_init (void);

void usb_handler_register (usb_handler_t handler);

#endif /*USB_DRV_H_*/

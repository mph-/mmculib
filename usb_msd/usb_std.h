/**
 * \file usb_std.h
 * Header: USB standard request handler
 * 
 * USB standard request handler functions
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
#ifndef USB_STD_H_
#define USB_STD_H_

/**
 * \name USB descriptor identifier
 * 
 */
//@{
#define USB_DEVICE_DESCRIPTOR                       0x01
#define USB_CONFIGURATION_DESCRIPTOR                0x02
#define USB_STRING_DESCRIPTOR                       0x03
#define USB_INTERFACE_DESCRIPTOR                    0x04
#define USB_ENDPOINT_DESCRIPTOR                     0x05
#define USB_DEVICE_QUALIFIER_DESCRIPTOR             0x06
#define USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR    0x07
#define USB_INTERFACE_POWER_DESCRIPTOR              0x08
//@}

/**
 * \name USB standard request identifier
 * 
 */
//@{
#define USB_GET_STATUS                 0x00
#define USB_CLEAR_FEATURE              0x01
// Reserved for futur use              0x02
#define USB_SET_FEATURE                0x03
// Reserved for futur use              0x04
#define USB_SET_ADDRESS                0x05
#define USB_GET_DESCRIPTOR             0x06
#define USB_SET_DESCRIPTOR             0x07
#define USB_GET_CONFIGURATION          0x08
#define USB_SET_CONFIGURATION          0x09
#define USB_GET_INTERFACE              0x0A
#define USB_SET_INTERFACE              0x0B
#define USB_SYNCH_FRAME                0x0C
//@}

/**
 * \name USB standard feature selectors
 * 
 */
//@{
#define USB_ENDPOINT_HALT              0x00
#define USB_DEVICE_REMOTE_WAKEUP       0x01
#define USB_TEST_MODE                  0x02
//@}

//! \brief Recipient is the whole device
#define USB_RECIPIENT_DEVICE                0x00

//! \brief Recipient is an interface
#define USB_RECIPIENT_INTERFACE             0x01

//! \brief Recipient is an endpoint
#define USB_RECIPIENT_ENDPOINT              0x02

//! \brief Defines a standard request
#define USB_STANDARD_REQUEST                0x00

//! \brief Defines a class request
#define USB_CLASS_REQUEST                   0x01

//! \brief Defines a vendor request
#define USB_VENDOR_REQUEST                  0x02

/**
 * Structure for data include in USB requests
 * 
 */
typedef struct
{
    unsigned char   bmRequestType:8;    //!< Characteristics of the request
    unsigned char   bRequest:8;         //!< Particular request
    unsigned short  wValue:16;          //!< Request-specific parameter
    unsigned short  wIndex:16;          //!< Request-specific parameter
    unsigned short  wLength:16;         //!< Length of data for the data phase

} __attribute__ ( (packed)) s_usb_request;


void usb_std_req_handler (s_usb_request *pSetup);

#endif /*USB_STD_H_*/

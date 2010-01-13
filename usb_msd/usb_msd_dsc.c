/**
 * \file usb_dsc.c
 * USB descriptor configuration 
 * 
 * USB descriptor configuration
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
#include "usb_msd_defs.h"
#include "usb_bot.h"
#include "usb_dsc.h"
#include "usb_std.h"

#ifndef USB_VENDOR_ID
#define USB_VENDOR_ID 0x03EB
#endif


#ifndef USB_PRODUCT_ID
#define USB_PRODUCT_ID 0x6202
#endif


#ifndef USB_RELEASE_ID
#define USB_RELEASE_ID 0x110
#endif


#ifndef USB_CURRENT_MA
#define USB_CURRENT_MA 100
#endif

// Descriptors
//! Device descriptor
static const S_usb_device_descriptor sDeviceDescriptor =
{
    sizeof(S_usb_device_descriptor), // Size of this descriptor in bytes
    USB_DEVICE_DESCRIPTOR,           // DEVICE Descriptor Type
    0x0200,                          // USB Specification 2.0
    0x00,                            // Class is specified in the interface descriptor.
    0x00,                            // Subclass is specified in the interface descriptor.
    0x00,                            // Protocol is specified in the interface descriptor.
    UDP_EP_CONTROL_SIZE,             // Maximum packet size for endpoint zero
    USB_VENDOR_ID,                   // Vendor ID
    USB_PRODUCT_ID,                  // Product ID
    USB_RELEASE_ID,                  // Device release number
    0x01,                            // Index 1: manufacturer string
    0x02,                            // Index 2: product string
    0x03,                            // Index 3: serial number string
    0x01                             // One possible configurations
};

//! Configuration descriptor
const S_bot_configuration_descriptor sConfigurationDescriptor = 
{
    // Configuration Descriptor
    {
        sizeof(S_usb_configuration_descriptor), // Size of this descriptor
        USB_CONFIGURATION_DESCRIPTOR,           // CONFIGURATION descriptor type
        sizeof(S_bot_configuration_descriptor), // Total size of descriptors
        0x01,                                   // One interface
        0x01,                                   // Configuration number 1
        0x00,                                   // No string description
        0x80,                                   // Device is self-powered
                                                // Remote wakeup not supported
        USB_CURRENT_MA / 2,                     // CMaxPower
    },
    // MSD Class Interface Descriptor
    {
        sizeof(S_usb_interface_descriptor), // Size of this descriptor in bytes
        USB_INTERFACE_DESCRIPTOR,           // INTERFACE descriptor type
        0x00,                               // Interface number 0
        0x00,                               // Setting 0
        2,                                  // Two endpoints used (excluding endpoint 0)
        MSD_INTF,                           // Mass storage class code
        MSD_INTF_SUBCLASS,                  // SCSI subclass code
        MSD_PROTOCOL,                       // Bulk-only transport protocol
        0x00                                // No string description
    },
    // Bulk_OUT Endpoint Descriptor
    {
        sizeof(S_usb_endpoint_descriptor),   // Size of this descriptor in bytes
        USB_ENDPOINT_DESCRIPTOR,             // ENDPOINT descriptor type
        0x01,                                // OUT endpoint, address 01h
        0x02,                                // Bulk endpoint
        USB_BOT_OUT_EP_SIZE,                 // Maximum packet size is 64 bytes
        0x00,                                // Must be 0 for full-speed bulk
    },
    // Bulk_IN Endpoint Descriptor
    {
        sizeof(S_usb_endpoint_descriptor), // Size of this descriptor in bytes
        USB_ENDPOINT_DESCRIPTOR,           // ENDPOINT descriptor type
        0x82,                              // IN endpoint, address 02h
        0x02,                              // Bulk endpoint
        USB_BOT_IN_EP_SIZE,                // Maximum packet size is 64 bytes
        0x00,                              // Must be 0 for full-speed bulk
    }
};

//! Device qualifier descriptor
static const S_usb_device_qualifier_descriptor sDeviceQualifierDescriptor =
{
   sizeof(S_usb_device_qualifier_descriptor), // Size of this descriptor in bytes
   USB_DEVICE_QUALIFIER_DESCRIPTOR,     //!< DEVICE_QUALIFIER descriptor type
   0x0200,                              //!< USB specification release number
   MSD_INTF,                            //!< Class code
   MSD_INTF_SUBCLASS,                   //!< Sub-class code
   MSD_PROTOCOL,                        //!< Protocol code
   UDP_EP_CONTROL_SIZE,                 //!< Control endpoint 0 max. packet size
   1,                                   //!< Number of possible configurations
   0                                    //!< Reserved for future use, must be 0
};

// String descriptors
//! \brief  Language ID
static const S_usb_language_id sLanguageID =
{
    USB_STRING_DESCRIPTOR_SIZE(1),
    USB_STRING_DESCRIPTOR,
    USB_LANGUAGE_ENGLISH_US
};


#define UU(X) USB_UNICODE(X)

#define USB_MANUFACTURER_STRING  UU('A'), UU('T'), UU('M'), UU('E'), UU('L')
#define USB_MANUFACTURER_LEN 5

#define USB_PRODUCT_STRING  UU('A'), UU('T'), UU('9'), UU('1'), UU('S'), UU('A'), UU('M'), UU('7'), UU('-'), UU('1'), UU('2'), UU('8'), UU(' '), UU('U'), UU('S'), UU('B'), UU(' '), UU('M'), UU('S'), UU('D'), UU(' '), UU('E'), UU('x'), UU('a'), UU('m'), UU('p'), UU('l'), UU('e'),
#define USB_PRODUCT_LEN 28

#define USB_SERIAL_STRING  UU('F'), UU('E'), UU('E'), UU('D'), UU('D'), UU('E'), UU('A'), UU('D'), UU('B'), UU('E'), UU('E'), UU('F')
#define USB_SERIAL_LEN 12


//! \brief  Manufacturer description
const char pManufacturer[] = 
{
    USB_STRING_DESCRIPTOR_SIZE(USB_MANUFACTURER_LEN),
    USB_STRING_DESCRIPTOR,
    USB_MANUFACTURER_STRING
};

//! \brief  Product descriptor
static const char pProduct[] =
{
    USB_STRING_DESCRIPTOR_SIZE(USB_PRODUCT_LEN),
    USB_STRING_DESCRIPTOR,
    USB_PRODUCT_STRING
};

//! \brief  Serial number
static const char pSerial[] = 
{

    USB_STRING_DESCRIPTOR_SIZE(USB_SERIAL_LEN),
    USB_STRING_DESCRIPTOR,
    USB_SERIAL_STRING
};

//! \brief  List of string descriptors used by the device
static const char *pStringDescriptors[] = 
{
    (char *) &sLanguageID,
    pManufacturer,
    pProduct,
    pSerial
};

//! \brief  List of descriptors used by the device
//! \see    S_std_descriptors
const S_std_descriptors sDescriptors =
{
    &sDeviceDescriptor,
    (S_usb_configuration_descriptor *) &sConfigurationDescriptor,
#ifdef USB_HIGHSPEED    
    &sDeviceQualifierDescriptor,
#endif    
    pStringDescriptors,
    0
};

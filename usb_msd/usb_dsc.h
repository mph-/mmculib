/**
 * \file usb_dsc.h
 * Header: USB descriptor configuration 
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
#ifndef USB_DSC_H_
#define USB_DSC_H_

#include "config.h"


/** Converts an ASCII character to its Unicode equivalent */
#define USB_UNICODE(a)                      (a), 0x00

/**
 * Calculates the size of a string descriptor given the number of ASCII
 * characters in it
 * 
 */
#define USB_STRING_DESCRIPTOR_SIZE(size)    ((size * 2) + 2)

/** English (United States) */
#define USB_LANGUAGE_ENGLISH_US     0x0409

/**
 * This descriptor structure is used to provide information on
 * various parameters of the device
 * 
 */
typedef struct 
{
   uint8_t  bLength;              //!< Size of this descriptor in bytes
   uint8_t  bDescriptorType;      //!< DEVICE descriptor type
   uint16_t bscUSB;               //!< USB specification release number
   uint8_t  bDeviceClass;         //!< Class code
   uint8_t  bDeviceSubClass;      //!< Subclass code
   uint8_t  bDeviceProtocol;      //!< Protocol code
   uint8_t  bMaxPacketSize0;      //!< Control endpoint 0 max. packet size
   uint16_t idVendor;             //!< Vendor ID
   uint16_t idProduct;            //!< Product ID
   uint16_t bcdDevice;            //!< Device release number
   uint8_t  iManufacturer;        //!< Index of manu. string descriptor
   uint8_t  iProduct;             //!< Index of prod. string descriptor
   uint8_t  iSerialNumber;        //!< Index of S.N.  string descriptor
   uint8_t  bNumConfigurations;   //!< Number of possible configurations
} __packed__ S_usb_device_descriptor;


/**
 * This is the standard configuration descriptor structure. It is used
 * to report the current configuration of the device.
 * 
 */ 
typedef struct
{
   uint8_t  bLength;              //!< Size of this descriptor in bytes
   uint8_t  bDescriptorType;      //!< CONFIGURATION descriptor type
   uint16_t wTotalLength;         //!< Total length of data returned
                                        //!< for this configuration
   uint8_t  bNumInterfaces;       //!< Number of interfaces for this
                                        //!< configuration
   uint8_t  bConfigurationValue;  //!< Value to use as an argument for
                                        //!< the Set Configuration request to
                                        //!< select this configuration
   uint8_t  iConfiguration;       //!< Index of string descriptor
                                        //!< describing this configuration
   uint8_t  bmAttibutes;          //!< Configuration characteristics
   uint8_t  bMaxPower;            //!< Maximum power consumption of the
                                        //!< device
} __packed__ S_usb_configuration_descriptor;


/**
 * Standard interface descriptor. Used to describe a specific interface
 * of a configuration.
 * 
 */
typedef struct
{
   uint8_t bLength;               //!< Size of this descriptor in bytes
   uint8_t bDescriptorType;       //!< INTERFACE descriptor type
   uint8_t bInterfaceNumber;      //!< Number of this interface
   uint8_t bAlternateSetting;     //!< Value used to select this alternate
                                        //!< setting
   uint8_t bNumEndpoints;         //!< Number of endpoints used by this
                                        //!< interface (excluding endpoint zero)
   uint8_t bInterfaceClass;       //!< Class code
   uint8_t bInterfaceSubClass;    //!< Sub-class
   uint8_t bInterfaceProtocol;    //!< Protocol code
   uint8_t iInterface;            //!< Index of string descriptor
                                        //!< describing this interface
} __packed__ S_usb_interface_descriptor;

/**
 * This structure is the standard endpoint descriptor. It contains
 * the necessary information for the host to determine the bandwidth
 * required by the endpoint.
 * 
 */
typedef struct
{
   uint8_t  bLength;              //!< Size of this descriptor in bytes
   uint8_t  bDescriptorType;      //!< ENDPOINT descriptor type
   uint8_t  bEndpointAddress;     //!< Address of the endpoint on the USB
                                        //!< device described by this descriptor
   uint8_t  bmAttributes;         //!< Endpoint attributes when configured
   uint16_t wMaxPacketSize;       //!< Maximum packet size this endpoint
                                        //!< is capable of sending or receiving
   uint8_t  bInterval;            //!< Interval for polling endpoint for
                                        //!< data transfers
} __packed__ S_usb_endpoint_descriptor;


/**
 * The device qualifier structure provide information on a high-speed
 * capable device if the device was operating at the other speed.
 * 
 */ 
typedef struct
{
   uint8_t  bLength;              //!< Size of this descriptor in bytes
   uint8_t  bDescriptorType;      //!< DEVICE_QUALIFIER descriptor type
   uint16_t bscUSB;               //!< USB specification release number
   uint8_t  bDeviceClass;         //!< Class code
   uint8_t  bDeviceSubClass;      //!< Sub-class code
   uint8_t  bDeviceProtocol;      //!< Protocol code
   uint8_t  bMaxPacketSize0;      //!< Control endpoint 0 max. packet size
   uint8_t  bNumConfigurations;   //!< Number of possible configurations
   uint8_t  bReserved;            //!< Reserved for future use, must be 0

} __packed__ S_usb_device_qualifier_descriptor;


/**
 * The S_usb_language_id structure represents the string descriptor
 * zero, used to specify the languages supported by the device. This
 * structure only define one language ID.
 * 
 */
typedef struct
{
   uint8_t  bLength;               //!< Size of this descriptor in bytes
   uint8_t  bDescriptorType;       //!< STRING descriptor type
   uint16_t wLANGID;               //!< LANGID code zero
} __packed__ S_usb_language_id;


/**
 * Configuration descriptor used by the MSD driver
 * 
 * \see    S_usb_configuration_descriptor
 * \see    S_usb_interface_descriptor
 * \see    S_usb_endpoint_descriptor
 * 
 */
typedef struct
{
    S_usb_configuration_descriptor sConfigurationDescriptor; //!< Configuration descriptor
    S_usb_interface_descriptor     sInterface;     //!< Interface descriptor
    S_usb_endpoint_descriptor      sBulkOut;       //!< Bulk OUT endpoint
    S_usb_endpoint_descriptor      sBulkIn;        //!< Bulk IN endpoint

} S_bot_configuration_descriptor;

/** List of standard descriptors used by the device */
typedef struct  {

    //! Device descriptor
    const S_usb_device_descriptor           *pDevice;
    //! Configuration descriptor
    const S_usb_configuration_descriptor    *pConfiguration;
#ifdef USB_HIGHSPEED
    //! Device qualifier descriptor
    const S_usb_device_qualifier_descriptor *pQualifier;
#endif    
    //! List of string descriptors
    const char                              **pStrings;
    //! List of endpoint descriptors
    const S_usb_endpoint_descriptor         **pEndpoints;
} S_std_descriptors;

extern const S_std_descriptors sDescriptors;

#endif  /*USB_DSC_H_*/

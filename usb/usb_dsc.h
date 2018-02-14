#ifndef USB_DSC_H_
#define USB_DSC_H_

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"


/**
 * Calculates the size of a string descriptor given the number of ASCII
 * characters
 * 
 */
#define USB_STRING_DESCRIPTOR_SIZE(size)    ((size * 2) + 2)

/** English (United States) */
#define USB_LANGUAGE_ENGLISH_US     0x0409



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
} __packed__ usb_dsc_dev_t;


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
} __packed__ usb_dsc_cfg_t;


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
} __packed__ usb_dsc_if_t;

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
} __packed__ usb_dsc_ep_t;


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
} __packed__ usb_dsc_dev_qualifier_t;


/**
 * The usb_dsc_lang_t structure represents the string descriptor
 * zero, used to specify the languages supported by the device.  This
 * structure only defines one language ID.
 * 
 */
typedef struct
{
   uint8_t  bLength;               //!< Size of this descriptor in bytes
   uint8_t  bDescriptorType;       //!< STRING descriptor type
   uint16_t wLANGID;               //!< LANGID code zero
} __packed__ usb_dsc_lang_t;


/** List of standard descriptors used by the device */
typedef struct
{
    //! Configuration descriptor
    const usb_dsc_cfg_t *config;
#ifdef USB_HIGHSPEED
    //! Device qualifier descriptor
    const usb_dsc_dev_qualifier_t *qualifier;
#endif    
    //! List of string descriptors
    const char **strings;
    //! List of endpoint descriptors
    const usb_dsc_ep_t **endpoints;
} usb_dsc_t;



#ifdef __cplusplus
}
#endif    
#endif  /*USB_DSC_H_*/


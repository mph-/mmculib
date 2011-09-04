#include "usb.h"
#include "usb_msd_defs.h"
#include "usb_dsc.h"


#ifndef USB_CURRENT_MA
#define USB_CURRENT_MA 100
#endif


/**
 * Configuration descriptor used by the MSD driver
 * 
 * \see    usb_dsc_cfg_t
 * \see    usb_dsc_if_t
 * \see    usb_dsc_ep_t
 * 
 */
typedef struct
{
    usb_dsc_cfg_t sConfiguration; //!< Configuration descriptor
    usb_dsc_if_t  sInterface;     //!< Interface descriptor
    usb_dsc_ep_t  sBulkOut;       //!< Bulk OUT endpoint
    usb_dsc_ep_t  sBulkIn;        //!< Bulk IN endpoint
} usb_msd_cfg_descriptor_t;


//! Configuration descriptor  (note first entry specifies total size)

static const usb_msd_cfg_descriptor_t usb_msd_cfg_descriptor = 
{
    // Configuration Descriptor
    {
        sizeof (usb_dsc_cfg_t),        // Size of descriptor in bytes
        USB_CONFIGURATION_DESCRIPTOR,  // CONFIGURATION descriptor type
        sizeof (usb_msd_cfg_descriptor_t), // Total size of descriptors
        0x01,                          // One interface
        0x01,                          // Configuration number 1
        0x00,                          // No string description
        0x80,                          // Device is self-powered
                                       // Remote wakeup not supported
        USB_CURRENT_MA / 2,            // CMaxPower
    },
    // MSD Class Interface Descriptor
    {
        sizeof (usb_dsc_if_t),         // Size of descriptor in bytes
        USB_INTERFACE_DESCRIPTOR,      // INTERFACE descriptor type
        0x00,                          // Interface number 0
        0x00,                          // Setting 0
        2,                             // Two endpoints used (excluding endpoint 0)
        MSD_INTF,                      // Mass storage class code
        MSD_INTF_SUBCLASS,             // SCSI subclass code
        MSD_PROTOCOL,                  // Bulk-only transport protocol
        0x00                           // No string description
    },
    // Bulk_OUT Endpoint Descriptor
    {
        sizeof (usb_dsc_ep_t),         // Size of descriptor in bytes
        USB_ENDPOINT_DESCRIPTOR,       // ENDPOINT descriptor type
        UDP_EP_OUT | UDP_EP_DIR_OUT,   // OUT endpoint
        0x02,                          // Bulk endpoint
        UDP_EP_OUT_SIZE,               // Maximum packet size is 64 bytes
        0x00,                          // Must be 0 for full-speed bulk
    },
    // Bulk_IN Endpoint Descriptor
    {
        sizeof (usb_dsc_ep_t),         // Size of descriptor in bytes
        USB_ENDPOINT_DESCRIPTOR,       // ENDPOINT descriptor type
        UDP_EP_IN | UDP_EP_DIR_IN,     // IN endpoint
        0x02,                          // Bulk endpoint
        UDP_EP_IN_SIZE,                // Maximum packet size is 64 bytes
        0x00,                          // Must be 0 for full-speed bulk
    }
};

//! Device qualifier descriptor
static const usb_dsc_dev_qualifier_t sDeviceQualifierDescriptor =
{
   sizeof (usb_dsc_dev_qualifier_t),    // Size of descriptor in bytes
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
static const usb_dsc_lang_t sLanguageID =
{
    USB_STRING_DESCRIPTOR_SIZE (1),
    USB_STRING_DESCRIPTOR,
    USB_LANGUAGE_ENGLISH_US
};


/** Converts an ASCII character to its Unicode equivalent */
#define USB_UNICODE(X) (X), 0x00
#define UU(X) USB_UNICODE (X)

#define USB_MANUFACTURER_STRING  UU ('A'), UU ('T'), UU ('M'), UU ('E'), UU ('L')
#define USB_MANUFACTURER_LEN 5

#define USB_PRODUCT_STRING  UU ('A'), UU ('T'), UU ('9'), UU ('1'), UU ('S'), UU ('A'), UU ('M'), UU ('7'), UU ('-'), UU ('1'), UU ('2'), UU ('8'), UU (' '), UU ('U'), UU ('S'), UU ('B'), UU (' '), UU ('M'), UU ('S'), UU ('D'), UU (' '), UU ('E'), UU ('x'), UU ('a'), UU ('m'), UU ('p'), UU ('l'), UU ('e'),
#define USB_PRODUCT_LEN 28

#define USB_SERIAL_STRING  UU ('F'), UU ('E'), UU ('E'), UU ('D'), UU ('D'), UU ('E'), UU ('A'), UU ('D'), UU ('B'), UU ('E'), UU ('E'), UU ('F')
#define USB_SERIAL_LEN 12


//! \brief  Manufacturer description
const char pManufacturer[] = 
{
    USB_STRING_DESCRIPTOR_SIZE (USB_MANUFACTURER_LEN),
    USB_STRING_DESCRIPTOR,
    USB_MANUFACTURER_STRING
};

//! \brief  Product descriptor
static const char pProduct[] =
{
    USB_STRING_DESCRIPTOR_SIZE (USB_PRODUCT_LEN),
    USB_STRING_DESCRIPTOR,
    USB_PRODUCT_STRING
};

//! \brief  Serial number
// The serial number shall contain at least 12 valid digits,
// represented as a UNICODE string. The last 12 digits of the
// serial number shall be unique to each USB idVendor and idProduct pair.
// Allowable digits are 0--9 and A--F.
static const char pSerial[] = 
{
    USB_STRING_DESCRIPTOR_SIZE (USB_SERIAL_LEN),
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
//! \see    usb_dsc_t
const usb_dsc_t usb_msd_descriptors =
{
    (usb_dsc_cfg_t *) &usb_msd_cfg_descriptor,
#ifdef USB_HIGHSPEED    
    &sDeviceQualifierDescriptor,
#endif    
    pStringDescriptors,
    0
};

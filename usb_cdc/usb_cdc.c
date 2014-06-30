#include "config.h"
#include "usb_cdc.h"
#include "usb_dsc.h"
#include "usb.h"


/* CDC communication device class. 

   Using sudo modprobe usbserial vendor=0x03EB product=0x6124
   will create a tty device such as /dev/ttyUSB0
*/

#ifndef USB_CURRENT_MA
#define USB_CURRENT_MA 100
#endif


#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif


static const char usb_cdc_cfg_descriptor[] = 
{
    /* ============== CONFIGURATION 1 =========== */
    /* Configuration 1 descriptor */
    0x09,   // CbLength
    0x02,   // CbDescriptorType
    0x43,   // CwTotalLength 2 EP + Control
    0x00,
    0x02,   // CbNumInterfaces
    0x01,   // CbConfigurationValue
    0x00,   // CiConfiguration
    0xC0,   // CbmAttributes 0xA0
    USB_CURRENT_MA / 2,   // CMaxPower
    
    /* Communication Class Interface Descriptor Requirement */
    0x09, // bLength
    0x04, // bDescriptorType
    0x00, // bInterfaceNumber
    0x00, // bAlternateSetting
    0x01, // bNumEndpoints
    0x02, // bInterfaceClass
    0x02, // bInterfaceSubclass
    0x00, // bInterfaceProtocol
    0x00, // iInterface
    
    /* Header Functional Descriptor */
    0x05, // bFunction Length
    0x24, // bDescriptor type: CS_INTERFACE
    0x00, // bDescriptor subtype: Header Func Desc
    0x10, // bcdCDC:1.1
    0x01,
    
    /* ACM Functional Descriptor */
    0x04, // bFunctionLength
    0x24, // bDescriptor Type: CS_INTERFACE
    0x02, // bDescriptor Subtype: ACM Func Desc
    0x00, // bmCapabilities
    
    /* Union Functional Descriptor */
    0x05, // bFunctionLength
    0x24, // bDescriptorType: CS_INTERFACE
    0x06, // bDescriptor Subtype: Union Func Desc
    0x00, // bMasterInterface: Communication Class Interface
    0x01, // bSlaveInterface0: Data Class Interface
    
    /* Call Management Functional Descriptor */
    0x05, // bFunctionLength
    0x24, // bDescriptor Type: CS_INTERFACE
    0x01, // bDescriptor Subtype: Call Management Func Desc
    0x00, // bmCapabilities: D1 + D0
    0x01, // bDataInterface: Data Class Interface 1
    
    /* Endpoint 1 descriptor */
    0x07,   // bLength
    0x05,   // bDescriptorType
    0x83,   // bEndpointAddress, Endpoint 03 - IN
    0x03,   // bmAttributes      INT
    0x08,   // wMaxPacketSize
    0x00,
    0xFF,   // bInterval
    
    /* Data Class Interface Descriptor Requirement */
    0x09, // bLength
    0x04, // bDescriptorType
    0x01, // bInterfaceNumber
    0x00, // bAlternateSetting
    0x02, // bNumEndpoints
    0x0A, // bInterfaceClass
    0x00, // bInterfaceSubclass
    0x00, // bInterfaceProtocol
    0x00, // iInterface
    
    /* First alternate setting */
    /* Endpoint 1 descriptor */
    0x07,   // bLength
    0x05,   // bDescriptorType
    0x01,   // bEndpointAddress, Endpoint 01 - OUT
    0x02,   // bmAttributes      BULK
    UDP_EP_OUT_SIZE,   // wMaxPacketSize
    0x00,
    0x00,   // bInterval
    
    /* Endpoint 2 descriptor */
    0x07,   // bLength
    0x05,   // bDescriptorType
    0x82,   // bEndpointAddress, Endpoint 02 - IN
    0x02,   // bmAttributes      BULK
    UDP_EP_IN_SIZE,   // wMaxPacketSize
    0x00,
    0x00    // bInterval
};


static const usb_descriptors_t usb_cdc_descriptors =
{
    (usb_dsc_cfg_t *) &usb_cdc_cfg_descriptor,
    0,
    0
};



/* CDC Class Specific Request Code */
#define GET_LINE_CODING               0x21A1
#define SET_LINE_CODING               0x2021
#define SET_CONTROL_LINE_STATE        0x2221


typedef struct 
{
    unsigned int dwDTERRate;
    char bCharFormat;
    char bParityType;
    char bDataBits;
} USB_CDC_LINE_CODING;


static USB_CDC_LINE_CODING line_coding =
{
    115200, // baudrate
    0,      // 1 Stop Bit
    0,      // None Parity
    8
};     // 8 Data bits


static usb_cdc_dev_t usb_cdc_dev;


static bool
usb_cdc_request_handler (usb_t usb, usb_setup_t *setup)
{
    switch ((setup->request << 8) | setup->type)
    {
    case SET_LINE_CODING:
        usb_control_gobble (usb);

        usb_control_write_zlp (usb);
        break;

    case GET_LINE_CODING:
        usb_control_write (usb, &line_coding,
                           MIN (sizeof (line_coding), setup->length));
        break;

    case SET_CONTROL_LINE_STATE:
        usb_control_write_zlp (usb);
        break;

    default:
        // Forward request to standard handler
        return 0;
    }
    return 1;
}


usb_cdc_size_t
usb_cdc_read (usb_cdc_t usb_cdc, void *buffer, usb_cdc_size_t length)
{
    return usb_read (usb_cdc->usb, buffer, length);
}


/* Checks if at least one character can be read without
   blocking.  */
bool
usb_cdc_read_ready_p (usb_cdc_t usb_cdc)
{
    return usb_read_ready_p (usb_cdc->usb);
}


usb_cdc_size_t
usb_cdc_write (usb_cdc_t usb_cdc, const void *buffer, usb_cdc_size_t length)
{
    return usb_write (usb_cdc->usb, buffer, length);
}


bool
usb_cdc_configured_p (usb_cdc_t usb_cdc)
{
    return usb_configured_p (usb_cdc->usb);
}


void
usb_cdc_shutdown (void)
{
    usb_shutdown ();
}


usb_cdc_t
usb_cdc_init (void)
{
    usb_cdc_t usb_cdc = &usb_cdc_dev;

    usb_cdc->usb = usb_init (&usb_cdc_descriptors, 
                             (void *)usb_cdc_request_handler);

    return usb_cdc;
}


/** Read character.  This blocks until the character can be read.  */
int8_t
usb_cdc_getc (usb_cdc_t usb_cdc)
{
    uint8_t ch;

    usb_cdc_read (usb_cdc, &ch, sizeof (ch));
    return ch;
}


/** Write character.  This blocks until the character can be
    written.  */
int8_t
usb_cdc_putc (usb_cdc_t usb_cdc, char ch)
{
    if (ch == '\n')
        usb_cdc_putc (usb_cdc, '\r');    

    usb_cdc_write (usb_cdc, &ch, sizeof (ch));
    return ch;
}


/** Write string.  This blocks until the string is buffered.  */
int8_t
usb_cdc_puts (usb_cdc_t usb_cdc, const char *str)
{
    while (*str)
        usb_cdc_putc (usb_cdc, *str++);
    return 1;
}


/** Return non-zero if configured.  */
bool
usb_cdc_update (void)
{
    /* This is needed to signal USB device.  */
    return usb_poll ((&usb_cdc_dev)->usb);
}

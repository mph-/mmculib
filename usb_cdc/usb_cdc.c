#include "config.h"
#include "usb_cdc.h"
#include "usb_dsc.h"
#include "usb.h"
#include <stdlib.h>


/* CDC communication device class.

   Using sudo modprobe usbserial vendor=0x03EB product=0x6124
   will create a tty device such as /dev/ttyUSB0
   or /dev/ttyACM0

   Writing works by:
   1. copying data to a ring buffer
   2. using asynchronous I/O to transmit a block from the ring buffer
      (without wrap-around) to the USB driver
   3. the asynchronous callback repeats step 2 until the ring buffer is empty.
*/

#ifndef USB_CURRENT_MA
#define USB_CURRENT_MA 100
#endif

#ifndef USB_CDC_TX_RING_SIZE
#define USB_CDC_TX_RING_SIZE 80
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

        // The value field of the message indicates that the terminal
        // is ready to receive.

        // See SiLabs app note AN758 for a description of this field.
        usb_cdc_dev.connected = setup->value & 0x1;
        break;

    default:
        // Forward request to standard handler
        return 0;
    }
    return 1;
}


/* Checks if at least one character can be read without
   blocking.  */
bool
usb_cdc_read_ready_p (usb_cdc_t usb_cdc)
{
    return usb_read_ready_p (usb_cdc->usb);
}


static int16_t
usb_cdc_read_nonblock (usb_cdc_t usb_cdc, void *data, uint16_t size)
{
    int ret;

    ret = usb_read_nonblock (usb_cdc->usb, data, size);

    if (ret == 0)
    {
        errno = EAGAIN;
        return -1;
    }
    return ret;
}


static void
usb_cdc_write_next (usb_cdc_dev_t *dev);


static void
usb_cdc_write_callback (void *usb_cdc, usb_transfer_t *transfer)
{
    usb_cdc_dev_t *dev = usb_cdc;

    ring_read_advance (&dev->tx_ring, transfer->transferred);
    dev->writing = 0;

    if (transfer->status != USB_STATUS_SUCCESS)
        return;

    usb_cdc_write_next (dev);
}


static void
usb_cdc_write_next (usb_cdc_dev_t *dev)
{
    int read_num;

    // The writing flag indicates aysnc I/O is in operation.   It
    // is cleared by the callback running in interrupt context.

    // There is a potential race condition here where the ISR may
    // change the flag after it has been read.  However, this should
    // have no effect.
    //  Read value   changed value
    //  0            0               start next transfer
    //  0            1               cannot happen, no transfer in progress
    //  1            1               transfer in progress, nothing to do
    //  1            0               transfer finished, nothing to do
    if (dev->writing)
        return;

    // Determine number of bytes able to be read without wrap around.
    read_num = ring_read_num_nowrap (&dev->tx_ring);
    if (read_num == 0)
        return;

    dev->writing = 1;
    if (usb_write_async (dev->usb, dev->tx_ring.out, read_num,
                         usb_cdc_write_callback, dev) != USB_STATUS_SUCCESS)
        dev->writing = 0;
}


/** Write up to size bytes but return if have to block.  */
static ssize_t
usb_cdc_write_nonblock (usb_cdc_t usb_cdc, const void *data, size_t size)
{
    int ret;
    usb_cdc_dev_t *dev = usb_cdc;

    ret = ring_write (&dev->tx_ring, data, size);

    if (ret == 0)
    {
        errno = EAGAIN;
        ret = -1;
    }

    usb_cdc_write_next (dev);
    return ret;
}


/** Write size bytes.  Block until all the bytes have been transferred
    to the transmit ring buffer or until timeout occurs.  */
ssize_t
usb_cdc_write (void *usb_cdc, const void *data, size_t size)
{
    usb_cdc_dev_t *dev = usb_cdc;

    return sys_write_timeout (usb_cdc, data, size, dev->write_timeout_us,
                             (void *)usb_cdc_write_nonblock);
}


/** Read size bytes.  Block until all the bytes have been read or
    until timeout occurs.  */
ssize_t
usb_cdc_read (void *usb_cdc, void *data, size_t size)
{
    usb_cdc_dev_t *dev = usb_cdc;

    /* Ignore size and read only one char to avoid timeout resetting
       endpoint.  */
    size = 1;

    return sys_read_timeout (usb_cdc, data, size, dev->read_timeout_us,
                             (void *)usb_cdc_read_nonblock);
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
usb_cdc_init (const usb_cdc_cfg_t *cfg)
{
    usb_cdc_t dev = &usb_cdc_dev;
    char *buffer;

    dev->read_timeout_us = cfg->read_timeout_us;
    dev->write_timeout_us = cfg->write_timeout_us;
    dev->writing = 0;
    dev->connected = 0;

    buffer = malloc (USB_CDC_TX_RING_SIZE);
    if (!buffer)
        return 0;

    ring_init (&dev->tx_ring, buffer, USB_CDC_TX_RING_SIZE);

    dev->usb = usb_init (&usb_cdc_descriptors,
                         (void *)usb_cdc_request_handler);

    return dev;
}


int
usb_cdc_getc (usb_cdc_t usb_cdc)
{
    char ret = 0;

    if (usb_cdc_read (usb_cdc, &ret, sizeof (ret)) < 0)
        return -1;

    if (ret == '\r')
        ret = '\n';

    return ret;
}


int
usb_cdc_putc (usb_cdc_t usb_cdc, char ch)
{
    if (ch == '\n')
        usb_cdc_putc (usb_cdc, '\r');

    if (usb_cdc_write (usb_cdc, &ch, sizeof (ch)) < 0)
        return -1;
    return ch;
}


int
usb_cdc_puts (usb_cdc_t usb_cdc, const char *str)
{
    while (*str)
        if (usb_cdc_putc (usb_cdc, *str++) < 0)
            return -1;
    return 1;
}


/** Return non-zero if configured.  */
bool
usb_cdc_update (void)
{
    /* This is needed to signal USB device.  */
    bool ret = usb_poll ((&usb_cdc_dev)->usb);

    /* USB disconnect == no terminal connected. */
    if (!ret && usb_cdc_dev.connected)
        usb_cdc_dev.connected = 0;

    return ret;
}


const sys_file_ops_t usb_cdc_file_ops =
{
    .read = usb_cdc_read,
    .write = usb_cdc_write
};

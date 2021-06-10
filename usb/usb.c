#include "usb.h"
#include "usb_std.h"
#include "trace.h"
#include "ring.h"


/* This module is mostly a wrapper for the device dependent UDP.
   It also handles setup and configuration requests. 

   This module must be completely device independent.  The udp module
   handles all the device dependent stuff.
 */


#ifndef USB_VENDOR_ID
#define USB_VENDOR_ID 0x03EB
#endif


#ifndef USB_PRODUCT_ID
//#define USB_PRODUCT_ID 0x6124
#define USB_PRODUCT_ID 0x6202
#endif


#ifndef USB_RELEASE_ID
#define USB_RELEASE_ID 0x110
#endif


#ifndef USB_CURRENT_MA
#define USB_CURRENT_MA 100
#endif


#ifndef TRACE_USB_ERROR
#define TRACE_USB_ERROR(...)
#endif


#ifndef TRACE_USB_INFO
#define TRACE_USB_INFO(...)
#endif


#ifndef TRACE_USB_DEBUG
#define TRACE_USB_DEBUG(...)
#endif


#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif


#ifndef HIGH_BYTE
#define HIGH_BYTE(v) (((v) >> 8) & 0xff)
#endif


#ifndef LOW_BYTE
#define LOW_BYTE(v) ((v) & 0xff)
#endif

// Descriptors
//! Device descriptor
static const usb_dsc_dev_t devDescriptor =
{
    sizeof (usb_dsc_dev_t),          // Size of this descriptor in bytes
    USB_DEVICE_DESCRIPTOR,           // DEVICE Descriptor Type
    0x0200,                          // USB specification 2.0 in BCD
    0x02,                            // Class is specified in the interface descriptor.
    0x02,                            // Subclass is specified in the interface descriptor.
    0x00,                            // Protocol is specified in the interface descriptor.
    UDP_EP_CONTROL_SIZE,             // Maximum packet size for endpoint zero
    USB_VENDOR_ID,                   // Vendor ID
    USB_PRODUCT_ID,                  // Product ID
    USB_RELEASE_ID,                  // Device release number
    0x01,                            // Index 1: manufacturer string
    0x02,                            // Index 2: product string
    0x03,                            // Index 3: serial number string
    0x01                             // One possible configuration
};



void 
usb_control_write (usb_t usb, const void *buffer, usb_size_t length)
{
    udp_write_async (usb->udp, UDP_EP_CONTROL, buffer, length, 0, 0);
}


void
usb_control_gobble (usb_t usb)
{
    return udp_control_gobble (usb->udp);
}


void
usb_control_write_zlp (usb_t usb)
{
    udp_write_async (usb->udp, UDP_EP_CONTROL, 0, 0, 0, 0);
}


static void
usb_control_write_zlp_callback (usb_t usb, udp_callback_t callback)
{
    udp_write_async (usb->udp, UDP_EP_CONTROL, 0, 0, callback, usb->udp);
}


void
usb_control_stall (usb_t usb)
{
    udp_stall (usb->udp, UDP_EP_CONTROL);
}


bool usb_halt (usb_t usb, udp_ep_t endpoint, bool halt)
{
    return udp_halt (usb->udp, endpoint, halt);
}


bool usb_halt_p (usb_t usb, udp_ep_t endpoint)
{
    return udp_halt_p (usb->udp, endpoint);
}


static void
usb_std_get_descriptor (usb_t usb, udp_setup_t *setup)
{
    // Check which descriptor was requested      
    switch (HIGH_BYTE (setup->value)) 
    {
    case USB_DEVICE_DESCRIPTOR:
        TRACE_DEBUG (USB, "USB:Dev\n");
        usb_control_write (usb, usb->dev_descriptor, 
                           MIN (usb->dev_descriptor->bLength, 
                                setup->length));    
        break;
        
    case USB_CONFIGURATION_DESCRIPTOR:
        TRACE_DEBUG (USB, "USB:Cfg\n");
        usb_control_write (usb, usb->descriptors->config, 
                           MIN (usb->descriptors->config->wTotalLength, 
                                setup->length));
        break;
        
    case USB_STRING_DESCRIPTOR:
        TRACE_DEBUG (USB, "USB:Str%d\n", LOW_BYTE (setup->value));

        if (!usb->descriptors->strings)
        {
            usb_control_stall (usb);
            break;
        }

        usb_control_write (usb, usb->descriptors->strings[LOW_BYTE (setup->value)], 
                           MIN (*(usb->descriptors->strings[LOW_BYTE (setup->value)]),
                                setup->length));    
        break;
        
    case USB_DEVICE_QUALIFIER_DESCRIPTOR:
        /* The host issues this when the USB version (bcdUSB) is set
           to 2.0 in the device descriptor, to examine the high-speed
           capability of the device.  For a full-speed device, stall
           this request.  */
        TRACE_DEBUG (USB, "USB:Qua\n");

#ifdef USB_HIGHSPEED
        if (!usb->descriptors->pQualifier)
        {
            usb_control_stall (usb);
            break;
        }

        usb_control_write (usb, usb->descriptors->pQualifier, 
                           MIN (usb->descriptors->pQualifier->bLength, 
                                setup->length));
#else
        usb_control_stall (usb);
#endif
        break;
        
    default:
        TRACE_ERROR (USB, "USB:Unknown gdesc 0x%02x\n", setup->request);
        /* Send stall for unsupported descriptor requests.  */
        usb_control_stall (usb);
    }
}


/**
 * USB standard request handler
 *
 * Service routine to handle all USB standard requests
 * 
 */
void
usb_std_request_handler (usb_t usb, udp_setup_t *setup)
{
    uint16_t temp;
    const usb_dsc_cfg_t *cfg_desc;
    
    // Handle supported standard device request see Table 9-3 in USB
    // specification Rev 2.0
    switch (setup->request)
    {
    case USB_GET_DESCRIPTOR:
        TRACE_INFO (USB, "USB:gDesc 0x%02x\n", setup->value);
        usb_std_get_descriptor (usb, setup);
        break;       

    case USB_SET_ADDRESS:
        TRACE_INFO (USB, "USB:sAddr 0x%02x\n", setup->value);
        usb_control_write_zlp_callback (usb, udp_address_set);
        break;            

    case USB_SET_CONFIGURATION:
        TRACE_INFO (USB, "USB:sCfg 0x%02x\n", setup->value);
        usb_control_write_zlp_callback (usb, udp_configuration_set);
        break;

    case USB_GET_CONFIGURATION:
        TRACE_INFO (USB, "USB:gCfg\n");
        // TODO, if have multiple configurations, select current one
        cfg_desc = usb->descriptors->config;
        usb_control_write (usb, cfg_desc, cfg_desc->bLength);
        break;

    case USB_CLEAR_FEATURE:
    case USB_SET_FEATURE:
        if (setup->request == USB_SET_FEATURE)
        {
            TRACE_INFO (USB, "USB:sFeat %u %u\n", setup->value, setup->index);
        }
        else
        {
            TRACE_INFO (USB, "USB:cFeat %u %u\n", setup->value, setup->index);
        }

        switch (setup->value)
        {
        case USB_ENDPOINT_HALT:
            TRACE_DEBUG (USB, "USB:Hlt\n");
            usb_halt (usb, LOW_BYTE (setup->index),
                      setup->request == USB_SET_FEATURE);
            usb_control_write_zlp (usb);
            break;
        
        case USB_DEVICE_REMOTE_WAKEUP:
            TRACE_DEBUG (USB, "USB:RmWak\n");
            usb_control_write_zlp (usb);
            break;
        
        default:
            TRACE_ERROR (USB, "USB:Bad Feature 0x%04x\n", setup->value);
            usb_control_stall (usb);
        }
        break;

    case USB_GET_STATUS:
        TRACE_INFO (USB, "USB:GetStatus\n");
        switch ((setup->type & 0x1F))
        {
        case USB_RECIPIENT_DEVICE:
            TRACE_INFO (USB, "USB:Dev\n");
            temp = 0; // status: No remote wakeup, bus powered
            usb_control_write (usb, &temp, sizeof (temp));
            break;
            
        case USB_RECIPIENT_ENDPOINT:
            TRACE_INFO (USB, "USB:Ept\n");
            // Retrieve the endpoint current status
            temp = usb_halt (usb, LOW_BYTE (setup->index), USB_GET_STATUS);
            // Return the endpoint status
            usb_control_write (usb, &temp, sizeof (temp)); 
            break;
            
        default:
            TRACE_ERROR (USB, "USB:Bad GetStatus 0x%02x\n", 
                         setup->type);
            usb_control_stall (usb);
        }
        break;

    case USB_GET_INTERFACE:
        TRACE_INFO (USB, "USB:gInt 0x%02x\n", setup->value);
        usb_control_stall (usb);
        break;

    case USB_SET_INTERFACE:
        TRACE_INFO (USB, "USB:sInt 0x%02x\n", setup->value);
        // Let's ignore this
        usb_control_write_zlp (usb);
        break;

    default:
        TRACE_ERROR (USB, "USB:Unknown req 0x%02x\n", setup->request);
        usb_control_stall (usb);
    }
}


bool
usb_read_ready_p (usb_t usb)
{
    return udp_read_ready_p (usb->udp);
}


usb_status_t
usb_write_async (usb_t usb, const void *buffer, 
                 unsigned int length, 
                 udp_callback_t callback, 
                 void *arg)
{
    return udp_write_async (usb->udp, UDP_EP_IN, buffer, length, callback, arg);
}


usb_status_t
usb_read_async (usb_t usb, void *buffer, 
                unsigned int length, 
                udp_callback_t callback, 
                void *arg)
{
    return udp_read_async (usb->udp, UDP_EP_OUT, buffer, length, callback, arg);
}


ssize_t
usb_read_nonblock (usb_t usb, void *buffer, size_t length)
{
    return udp_read_nonblock (usb->udp, buffer, length);
}


static void
usb_request_handler (usb_t usb, udp_setup_t *setup)
{
    /* Pass request to other handlers such as BOT or CDC before 
       handling standard requests.  */
    if (!usb->request_handler 
        || !usb->request_handler (usb, setup))
        usb_std_request_handler (usb, setup);
}


bool
usb_configured_p (usb_t usb)
{
    return udp_configured_p (usb->udp);
}


bool
usb_awake_p (usb_t usb)
{
    return udp_awake_p (usb->udp);
}


/** Return non-zero if configured.  */
bool
usb_poll (usb_t usb)
{
    return udp_poll (usb->udp);
}


void
usb_shutdown (void)
{
    udp_shutdown ();
}


usb_t usb_init (const usb_descriptors_t *descriptors,
                udp_request_handler_t request_handler)
{
    static usb_dev_t usb_dev;
    usb_t usb = &usb_dev;

    usb->udp = udp_init ((void *)usb_request_handler, usb);

    usb->descriptors = descriptors;
    usb->request_handler = request_handler;
    usb->dev_descriptor = &devDescriptor;

    return usb;
}

#include "usb.h"
#include "usb_std.h"
#include "trace.h"

/* This module is mostly a wrapper for the device dependent UDP.
   It also handles setup and configuration requests. 

   This module must be completely device independent.  The udp module
   handles all the device dependent stuff.
 */


#ifndef USB_VENDOR_ID
#define USB_VENDOR_ID 0x03EB
#endif


#ifndef USB_PRODUCT_ID
#define USB_PRODUCT_ID 0x6124
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
    sizeof(usb_dsc_dev_t), // Size of this descriptor in bytes
    USB_DEVICE_DESCRIPTOR,           // DEVICE Descriptor Type
    0x0200,                          // USB specification 2.0 in BCD
    0x00,                            // Class is specified in the interface descriptor.
    0x00,                            // Subclass is specified in the interface descriptor.
    0x00,                            // Protocol is specified in the interface descriptor.
    UDP_EP_CONTROL_SIZE,             // Maximum packet size for endpoint zero
    USB_VENDOR_ID,                   // Vendor ID
    USB_PRODUCT_ID,                  // Product ID
    USB_RELEASE_ID,                  // Device release number
    0x00,                            // Index 1: manufacturer string
    0x00,                            // Index 2: product string
    0x00,                            // Index 3: serial number string
    0x01                             // One possible configurations
};


/**
 * Callback for the STD_SetConfiguration function.
 * 
 */
static void
usb_std_configure_endpoints (usb_t usb, udp_setup_t *setup)
{
    unsigned int j;

    // Enter the configured state
    udp_set_configuration (usb->udp, setup);

    // Configure endpoints
    for (j = 0; j < UDP_EP_NUM; j++)
        udp_configure_endpoint (usb->udp, j);
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
        usb_control_write (usb, usb->descriptors->strings[LOW_BYTE (setup->value)], 
                           MIN (*(usb->descriptors->strings[LOW_BYTE (setup->value)]),
                                setup->length));    
        break;
        
    case USB_DEVICE_QUALIFIER_DESCRIPTOR:
        TRACE_DEBUG (USB, "USB:Qua\n");
#ifdef USB_HIGHSPEED
        usb_control_write (usb, usb->descriptors->pQualifier, 
                           MIN (usb->descriptors->pQualifier->bLength, 
                                setup->length));
#else
        usb_control_stall (usb);
#endif
        break;
        
    default:
        TRACE_ERROR (USB, "USB:Unknown gdesc 0x%02X\n", setup->request);
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
        TRACE_INFO (USB, "USB:gDesc 0x%02x\n", setup->request);
        usb_std_get_descriptor (usb, setup);
        break;       

    case USB_SET_ADDRESS:
        TRACE_INFO (USB, "USB:sAddr\n");
        usb_control_write_zlp (usb);
        udp_set_address (usb->udp, setup);
        break;            

    case USB_SET_CONFIGURATION:
        TRACE_INFO (USB, "USB:sCfg\n");
        usb_control_write_zlp (usb);
        usb_std_configure_endpoints (usb, setup);
        break;

    case USB_GET_CONFIGURATION:
        TRACE_INFO (USB, "USB:gCfg\n");
        cfg_desc = usb->descriptors->config;
        usb_control_write (usb, cfg_desc, cfg_desc->bLength);
        break;

    case USB_CLEAR_FEATURE:
        TRACE_INFO (USB, "USB:cFeat\n");
        switch (setup->value)
        {
        case USB_ENDPOINT_HALT:
            TRACE_INFO (USB, "USB:Hlt\n");
            usb_halt (usb, LOW_BYTE (setup->index), USB_CLEAR_FEATURE);
            usb_control_write_zlp (usb);
            break;
        
        case USB_DEVICE_REMOTE_WAKEUP:
            TRACE_INFO (USB, "USB:RmWak\n");
            usb_control_write_zlp (usb);
            break;
        
        default:
            TRACE_INFO (USB, "USB:Sta\n");
            usb_control_stall (usb);
        }
        break;

    case USB_SET_FEATURE:
        TRACE_INFO (USB, "USB:sFeat\n");
        switch (setup->value)
        {
        case USB_ENDPOINT_HALT:
            usb_halt (usb, LOW_BYTE (setup->index), USB_SET_FEATURE);
            usb_control_write_zlp (usb);
            break;
            
        case USB_DEVICE_REMOTE_WAKEUP:
            usb_control_write_zlp (usb);
            break;
            
        default:
            TRACE_INFO (USB, "USB:Bad SetFeature 0x%04X\n", setup->value);
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
            temp = (uint16_t) usb_halt (usb, LOW_BYTE (setup->index), 
                                        USB_GET_STATUS);
            // Return the endpoint status
            usb_control_write (usb, &temp, sizeof (temp));                        
            break;
            
        default:
            TRACE_INFO (USB, "USB:Bad GetStatus 0x%02X\n", 
                       setup->type);
            usb_control_stall (usb);
        }
        break;

    default:
        TRACE_INFO (USB, "USB:Bad req 0x%02X\n", setup->request);
        usb_control_stall (usb);
    }
}


void 
usb_control_write (usb_t usb, const void *buffer, usb_size_t length)
{
    return udp_control_write (usb->udp, buffer, length);
}


void
usb_control_gobble (usb_t usb)
{
    return udp_control_gobble (usb->udp);
}


void
usb_control_write_zlp (usb_t usb)
{
    return udp_control_write_zlp (usb->udp);
}


void
usb_control_stall (usb_t usb)
{
    return udp_control_stall (usb->udp);
}


bool usb_halt (usb_t usb, udp_ep_t endpoint, uint8_t request)
{
    return udp_halt (usb->udp, endpoint, request);
}


bool
usb_read_ready_p (usb_t usb)
{
    return udp_read_ready_p (usb->udp);
}


usb_size_t
usb_read (usb_t usb, void *buffer, usb_size_t length)
{
    return udp_read (usb->udp, buffer, length);
}


usb_size_t
usb_write (usb_t usb, const void *buffer, usb_size_t length)
{
    return udp_write (usb->udp, buffer, length);
}


usb_status_t
usb_write_async (usb_t usb, const void *buffer, 
                 unsigned int length, 
                 udp_callback_t callback, 
                 void *arg)
{
    return udp_write_async (usb->udp, buffer, length, callback, arg);
}


usb_status_t
usb_read_async (usb_t usb, void *buffer, 
                 unsigned int length, 
                 udp_callback_t callback, 
                 void *arg)
{
    return udp_read_async (usb->udp, buffer, length, callback, arg);
}


static void
usb_request_handler (usb_t usb, udp_setup_t *setup)
{
    /* Pass request to other handlers such as BOT or CDC first.  */
    if (!usb->request_handler 
        || !usb->request_handler (usb, setup))
        usb_std_request_handler (usb, setup);
}


bool
usb_configured_p (usb_t usb)
{
    return udp_configured_p (usb->udp);
}


void
usb_connect (usb_t usb)
{
    udp_connect (usb->udp);
}


bool
usb_awake_p (usb_t usb)
{
    return udp_awake_p (usb->udp);
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

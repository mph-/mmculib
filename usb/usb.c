#include "usb.h"
#include "usb_std.h"
#include "trace.h"

/* This module is mostly a wrapper for the device dependent UDP.
   It also handles setup and configuration requests. 

   This module must be completely device independent.
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

static const char devDescriptor[] =
{
    /* Device descriptor */
    0x12,   // bLength
    0x01,   // bDescriptorType
    0x10,   // bcdUSBL
    0x01,   //
    0x00,   // bDeviceClass (use class specified by interface)
    0x00,   // bDeviceSubclass
    0x00,   // bDeviceProtocol
    0x08,   // bMaxPacketSize0
    LOW_BYTE (USB_VENDOR_ID),    // idVendorL
    HIGH_BYTE (USB_VENDOR_ID),   //
    LOW_BYTE (USB_PRODUCT_ID),   // idProductL
    HIGH_BYTE (USB_PRODUCT_ID),  //
    LOW_BYTE (USB_RELEASE_ID),   // bcdDeviceL
    HIGH_BYTE (USB_RELEASE_ID),  //
    0x00,   // iManufacturer
    0x00,   // iProduct
    0x00,   // SerialNumber
    0x01    // bNumConfigs
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
        TRACE_INFO (USB, "USB:Dev\n");
        usb_control_write (usb, usb->dev_descriptor, 
                           MIN (usb->descriptors->pDevice->bLength, 
                                setup->length));    
        break;
        
    case USB_CONFIGURATION_DESCRIPTOR:
        TRACE_INFO (USB, "USB:Cfg\n");
        usb_control_write (usb, usb->descriptors->pConfiguration, 
                           MIN (usb->descriptors->pConfiguration->wTotalLength, 
                                setup->length));
        break;
        
    case USB_STRING_DESCRIPTOR:
        TRACE_INFO (USB, "USB:Str%d\n", LOW_BYTE (setup->value));
        usb_control_write (usb, usb->descriptors->pStrings[LOW_BYTE (setup->value)], 
                           MIN (*(usb->descriptors->pStrings[LOW_BYTE (setup->value)]),
                                setup->length));    
        break;
        
#ifdef USB_HIGHSPEED
    case USB_DEVICE_QUALIFIER_DESCRIPTOR:
        TRACE_INFO (USB, "USB:Qua\n");
        usb_control_write (usb, usb->descriptors->pQualifier, 
                           MIN (usb->descriptors->pQualifier->bLength, 
                                setup->length));
        break;
#endif
        
    default:
        TRACE_INFO (USB, "USB:Unknown GetDescriptor 0x%02X\n", 
                    setup->request );
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
    const S_usb_configuration_descriptor *cfg_desc;
    
    // Handle supported standard device request see Table 9-3 in USB
    // specification Rev 2.0
    switch (setup->request)
    {
    case USB_GET_DESCRIPTOR:
        TRACE_INFO (USB, "USB:gDesc\n");
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
        cfg_desc = usb->descriptors->pConfiguration;
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
            usb_control_write (usb, &temp, 2);
            break;
            
        case USB_RECIPIENT_ENDPOINT:
            TRACE_INFO (USB, "USB:Ept\n");
            // Retrieve the endpoint current status
            temp = (uint16_t) usb_halt (usb, LOW_BYTE (setup->index), 
                                        USB_GET_STATUS);
            // Return the endpoint status
            usb_control_write (usb, &temp, 2);                        
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
    /* Pass request to BOT or CDC handler first.  */
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
    usb->dev_descriptor = devDescriptor;

    return usb;
}

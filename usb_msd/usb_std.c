/**
 * \file usb_std.c
 * USB standard request handler 
 * 
 * Routine to handle all USB standard requests
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
#include "usb_trace.h"
#include "usb_dsc.h"
#include "usb_drv.h"
#include "usb_std.h"


#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define HBYTE(word) ((unsigned char) ((word) >> 8))
#define LBYTE(word) ((unsigned char) ((word) & 0x00FF))

/**
 * Callback for the STD_SetConfiguration function.
 * 
 */
static void
usb_std_configure_endpoints (void)
{
    unsigned int j;

    // Enter the configured state
    usb_set_configuration ();

    // Configure endpoints
    for (j = 0; j < NUM_OF_ENDPOINTS; j++)
        usb_configure_endpoint (j);
}


static void
usb_std_get_descriptor (s_usb_request *pSetup)
{
    // check which descriptor was requested      
    switch (HBYTE (pSetup->wValue)) 
    {
    case USB_DEVICE_DESCRIPTOR:
        TRACE_INFO (USB_DRV, "USB:Dev\n");
        usb_write (0, sDescriptors.pDevice,
                   MIN (sDescriptors.pDevice->bLength, pSetup->wLength), 0, 0);    
        break;
        
    case USB_CONFIGURATION_DESCRIPTOR:
        TRACE_INFO (USB_DRV, "USB:Cfg\n");
        usb_write (0, sDescriptors.pConfiguration,
                   MIN (sDescriptors.pConfiguration->wTotalLength, pSetup->wLength), 0, 0);
        break;
        
    case USB_STRING_DESCRIPTOR:
        TRACE_INFO (USB_DRV, "USB:Str%d\n", LBYTE (pSetup->wValue));
        usb_write (0, sDescriptors.pStrings[LBYTE (pSetup->wValue)],
                   MIN (* (sDescriptors.pStrings[LBYTE (pSetup->wValue)]),pSetup->wLength), 0, 0);    
        break;
        
#ifdef USB_HIGHSPEED
    case USB_DEVICE_QUALIFIER_DESCRIPTOR:
        TRACE_INFO (USB_DRV, "USB:Qua\n");
        usb_write (0, sDescriptors.pQualifier,
                   MIN (sDescriptors.pQualifier->bLength, pSetup->wLength), 0, 0);
        break;
#endif
        
    default:
        TRACE_INFO (USB_DRV, "USB:Unknown GetDescriptor 0x%02X\n",
                    pSetup->bRequest );
        /* Send stall for unsupported descriptor requests.  */
        usb_stall (0);
    }
}


/**
 * USB standard request handler
 *
 * Service routine to handle all USB standard requests
 * 
 */
void usb_std_req_handler (s_usb_request *pSetup)
{
    uint16_t temp; // for temporary data
    
    // Handle supported standard device request see Table 9-3 in USB
    // specification Rev 2.0
    switch (pSetup->bRequest)
    {
    case USB_GET_DESCRIPTOR:
        TRACE_INFO (USB_DRV, "USB:gDesc\n");
        usb_std_get_descriptor (pSetup);
        break;       

    case USB_SET_ADDRESS:
        TRACE_INFO (USB_DRV, "USB:sAddr\n");
        usb_send_zlp0 ((usb_callback_t) usb_set_address, 0);
        break;            

    case USB_SET_CONFIGURATION:
        TRACE_INFO (USB_DRV, "USB:sCfg\n");
        usb_send_zlp0 ((usb_callback_t) usb_std_configure_endpoints, 0);
        break;

    case USB_GET_CONFIGURATION:
        TRACE_INFO (USB_DRV, "USB:gCfg\n");
        if (ISSET (usb_device_state, USB_STATE_CONFIGURED))
            usb_configuration = 1;  // we are configured, use the only avail. configuration
        else
            usb_configuration = 0;  // No configuration active
        usb_write (0, (const void *)&usb_configuration, 1, 0, 0);
        break;

    case USB_CLEAR_FEATURE:
        TRACE_INFO (USB_DRV, "USB:cFeat\n");
        switch (pSetup->wValue)
        {
        case USB_ENDPOINT_HALT:
            TRACE_INFO (USB_DRV, "USB:Hlt\n");
            usb_halt (LBYTE (pSetup->wIndex), USB_CLEAR_FEATURE);
            usb_send_zlp0 (0,0);
            break;
        
        case USB_DEVICE_REMOTE_WAKEUP:
            TRACE_INFO (USB_DRV, "USB:RmWak\n");
            usb_send_zlp0 (0,0);
            break;
        
        default:
            TRACE_INFO (USB_DRV, "USB:Sta\n");
            usb_stall (0);
        }
        break;

    case USB_SET_FEATURE:
        TRACE_INFO (USB_DRV, "USB:sFeat\n");
        switch (pSetup->wValue)
        {
        case USB_ENDPOINT_HALT:
            usb_halt (LBYTE (pSetup->wIndex), USB_SET_FEATURE);
            usb_send_zlp0 (0,0);
            break;
            
        case USB_DEVICE_REMOTE_WAKEUP:
            usb_send_zlp0 (0,0);
            break;
            
        default:
            TRACE_INFO (USB_DRV, "USB:Bad SetFeature 0x%04X\n", pSetup->wValue);
            usb_stall (0);
        }
        break;

    case USB_GET_STATUS:
        TRACE_INFO (USB_DRV, "USB:GetStatus\n");
        switch ((pSetup->bmRequestType & 0x1F))
        {
        case USB_RECIPIENT_DEVICE:
            TRACE_INFO (USB_DRV, "USB:Dev\n");
            temp = 0; // status: No remote wakeup, bus powered
            usb_write (0, &temp, 2, 0, 0);
            break;
            
        case USB_RECIPIENT_ENDPOINT:
            TRACE_INFO (USB_DRV, "USB:Ept\n");
            // Retrieve the endpoint current status
            temp = (uint16_t) usb_halt (LBYTE (pSetup->wIndex), USB_GET_STATUS);
            // Return the endpoint status
            usb_write (0, &temp, 2, 0, 0);                        
            break;
            
        default:
            TRACE_INFO (USB_DRV, "USB:Bad GetStatus 0x%02X\n",
                       pSetup->bmRequestType);
            usb_stall (0);
        }
        break;

    default:
        TRACE_INFO (USB_DRV, "USB:Bad req 0x%02X\n", pSetup->bRequest);
        usb_stall (0);
    }
}

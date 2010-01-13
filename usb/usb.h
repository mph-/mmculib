#ifndef USB_H
#define USB_H

#include "config.h"
#include "udp.h"
#include "usb_dsc.h"
#include "usb_std.h"

typedef S_std_descriptors usb_descriptors_t;


// FIXME, these need a direct correspondence with udp_status_t
typedef enum
{
//! Last method has completed successfully
    USB_STATUS_SUCCESS = 0,
//! Method was aborted because the recipient (device, endpoint, ...) was busy
    USB_STATUS_LOCKED = 1,
//! Method was aborted because of abnormal status
    USB_STATUS_ABORTED = 2,
//! Method was aborted because the endpoint or the device has been reset
    USB_STATUS_RESET = 3,
//! Method status unknow
    USB_STATUS_UNKOWN = 4
} usb_status_t;


typedef struct usb_dev_struct
{
    udp_t udp;
    const char *dev_descriptor;
    const usb_descriptors_t *descriptors;
    udp_request_handler_t request_handler;
} usb_dev_t;


typedef usb_dev_t *usb_t;

typedef udp_size_t usb_size_t;

void usb_control_write (usb_t usb, const void *data, usb_size_t length);

void usb_control_gobble (usb_t usb);

void usb_control_write_zlp (usb_t usb);

void usb_control_stall (usb_t usb);

bool usb_halt (usb_t usb, udp_ep_t endpoint, uint8_t request);

bool usb_read_ready_p (usb_t usb);

usb_size_t usb_read (usb_t usb, void *buffer, usb_size_t length);

usb_size_t usb_write (usb_t usb, const void *buffer, usb_size_t length);

usb_status_t usb_write_async (usb_t usb, const void *buffer, 
                              unsigned int length, 
                              udp_callback_t callback, 
                              void *arg);

usb_status_t usb_read_async (usb_t usb, void *buffer, 
                             unsigned int length, 
                             udp_callback_t callback, 
                             void *arg);

bool usb_configured_p (usb_t usb);

void usb_connect (usb_t usb);

bool usb_awake_p (usb_t usb);

usb_t usb_init (const usb_descriptors_t *descriptors,
                udp_request_handler_t request_handler);

#endif

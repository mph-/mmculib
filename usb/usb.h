#ifndef USB_H
#define USB_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "udp.h"
#include "usb_dsc.h"
#include "usb_std.h"
#include "ring.h"

typedef usb_dsc_t usb_descriptors_t;

//! Completed successfully
#define USB_STATUS_SUCCESS UDP_STATUS_SUCCESS
//! Aborted because the recipient (device, endpoint, ...) was busy
#define USB_STATUS_BUSY UDP_STATUS_BUSY
//! Aborted because of abnormal status
#define USB_STATUS_ABORTED UDP_STATUS_ABORTED
//! Aborted because the endpoint or the device has been reset
#define USB_STATUS_RESET UDP_STATUS_RESET
//! Waiting completion of transfer
#define USB_STATUS_PENDING UDP_STATUS_PENDING

typedef udp_status_t usb_status_t;

typedef udp_setup_t usb_setup_t;

typedef udp_transfer_t usb_transfer_t;

typedef udp_callback_t usb_callback_t;

typedef bool (*usb_request_handler_t) (void *arg, usb_setup_t *setup);

typedef struct usb_dev_struct
{
    udp_t udp;
    const usb_dsc_dev_t *dev_descriptor;
    const usb_descriptors_t *descriptors;
    usb_request_handler_t request_handler;
} usb_dev_t;


typedef usb_dev_t *usb_t;

typedef udp_size_t usb_size_t;

void usb_control_write (usb_t usb, const void *data, usb_size_t length);

void usb_control_gobble (usb_t usb);

void usb_control_write_zlp (usb_t usb);

void usb_control_stall (usb_t usb);

bool usb_halt (usb_t usb, udp_ep_t endpoint, bool halt);

bool usb_halt_p (usb_t usb, udp_ep_t endpoint);

bool usb_read_ready_p (usb_t usb);

ssize_t usb_read_nonblock (usb_t usb, void *buffer, size_t length);

usb_status_t usb_write_async (usb_t usb, const void *buffer, 
                              unsigned int length, 
                              usb_callback_t callback, 
                              void *arg);

usb_status_t usb_read_async (usb_t usb, void *buffer, 
                             unsigned int length, 
                             usb_callback_t callback, 
                             void *arg);

/** Return non-zero if configured.  */
bool usb_poll (usb_t usb);

bool usb_configured_p (usb_t usb);

bool usb_awake_p (usb_t usb);

usb_t usb_init (const usb_descriptors_t *descriptors,
                udp_request_handler_t request_handler);


void usb_shutdown (void);


#ifdef __cplusplus
}
#endif    
#endif


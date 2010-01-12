#ifndef USB_CDC_H
#define USB_CDC_H

#include "config.h"
#include "usb.h"

typedef struct usb_cdc_struct
{
    usb_t usb;
} usb_cdc_dev_t;

typedef usb_cdc_dev_t *usb_cdc_t;

typedef usb_size_t usb_cdc_size_t;


extern usb_cdc_size_t
usb_cdc_write (usb_cdc_t usb_cdc, const void *buffer, usb_cdc_size_t length);


extern usb_cdc_size_t
usb_cdc_read (usb_cdc_t usb_cdc, void *buffer, usb_cdc_size_t length);


extern bool
usb_cdc_configured_p (usb_cdc_t dev);


extern void
usb_cdc_connect (usb_cdc_t dev);


extern void
usb_cdc_shutdown (void);


extern usb_cdc_t 
usb_cdc_init (void);


extern bool
usb_cdc_read_ready_p (usb_cdc_t usb_cdc);


/* Read character.  */
extern int8_t
usb_cdc_getc (usb_cdc_t usb_cdc);


/* Write character.  */
extern int8_t
usb_cdc_putc (usb_cdc_t usb_cdc, char ch);


/* Write string.  This blocks until the string is buffered.  */
extern int8_t
usb_cdc_puts (usb_cdc_t usb_cdc, const char *str);

#endif

#ifndef USB_CDC_H
#define USB_CDC_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "usb.h"
#include "ring.h"
    

typedef struct usb_cdc_struct
{
    usb_t usb;
    ring_t tx_ring;
    uint32_t read_timeout_us;
    uint32_t write_timeout_us;
    volatile bool writing;
    bool connected;
} usb_cdc_dev_t;

typedef usb_cdc_dev_t *usb_cdc_t;

typedef usb_size_t usb_cdc_size_t;

/** usb cdc configuration structure.  */
typedef struct
{
    /* Zero for non-blocking I/O.  */    
    uint32_t read_timeout_us;
    uint32_t write_timeout_us;
}
usb_cdc_cfg_t;


ssize_t
usb_cdc_write (void *usb_cdc, const void *buffer, size_t length);


ssize_t
usb_cdc_read (void *usb_cdc, void *buffer, size_t length);


bool
usb_cdc_configured_p (usb_cdc_t dev);


void
usb_cdc_connect (usb_cdc_t dev);


void
usb_cdc_shutdown (void);


usb_cdc_t 
usb_cdc_init (const usb_cdc_cfg_t *cfg);


bool
usb_cdc_read_ready_p (usb_cdc_t usb_cdc);


/** Read character.  */
int
usb_cdc_getc (usb_cdc_t usb_cdc);


/** Write character.  */
int
usb_cdc_putc (usb_cdc_t usb_cdc, char ch);


/** Write string.  This blocks until the string is buffered.  */
int
usb_cdc_puts (usb_cdc_t usb_cdc, const char *str);


/** Return non-zero if configured.  */
bool
usb_cdc_update (void);


extern const sys_file_ops_t usb_cdc_file_ops;


#ifdef __cplusplus
}
#endif    
#endif


#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "usb_cdc.h"
#include "tty.h"    

/** usb_serial configuration structure.  */
typedef struct
{
    usb_cdc_cfg_t usb_cdc;
    const char *devname;    
}
usb_serial_cfg_t;


typedef struct
{
    usb_cdc_t usb_cdc;        
    tty_t *tty;    
} usb_serial_t;
    
    
    
usb_serial_t *
usb_serial_init (const usb_serial_cfg_t *cfg);


void
usb_serial_shutdown (usb_serial_t *dev);


void
usb_serial_echo_set (usb_serial_t *dev, bool echo);
    

#ifdef __cplusplus
}
#endif    
#endif


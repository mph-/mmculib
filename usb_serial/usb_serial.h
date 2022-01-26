#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "usb_cdc.h"
#include "tty.h"    

/** usb_serial configuration structure.  */
typedef usb_cdc_cfg_t usb_serial_cfg_t;


typedef struct
{
    usb_cdc_t usb_cdc;        
    tty_t *tty;    
} usb_serial_t;
    
    
    
usb_serial_t *
usb_serial_init (const usb_serial_cfg_t *cfg, const char *devname);


void
usb_serial_shutdown (usb_serial_t *dev);


void
usb_serial_echo_set (usb_serial_t *dev, bool echo);
    

void
usb_serial_puts (usb_serial_t *dev, const char *str);


char *
usb_serial_gets (usb_serial_t *dev, char *buffer, int size);


int
usb_serial_stdio_init (void);
    
    
#ifdef __cplusplus
}
#endif    
#endif


#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

/** usb_serial configuration structure.  */
typedef struct
{
    /* Zero for non-blocking I/O.  */    
    uint32_t read_timeout_us;
    uint32_t write_timeout_us;
    bool echo;
    const char *devname;
}
usb_serial_cfg_t;

    
bool
usb_serial_init (const usb_serial_cfg_t *cfg);


#ifdef __cplusplus
}
#endif    
#endif


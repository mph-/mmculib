#include "usb_serial.h"
#include "usb_cdc.h"
#include "tty.h"
#include "sys.h"


bool
usb_serial_init (const usb_serial_cfg_t *cfg)
{
    tty_cfg_t usb_cdc_tty_cfg =
        {
            .read = (void *)usb_cdc_read,
            .write = (void *)usb_cdc_write,
            .linebuffer_size = 80
        };
    const usb_cdc_cfg_t usb_cdc_cfg =
        {
            .read_timeout_us = cfg->read_timeout_us,
            .write_timeout_us = cfg->write_timeout_us,
        };
    usb_cdc_t *usb_cdc;        
    tty_t *usb_cdc_tty;    

    usb_cdc = usb_cdc_init (&usb_cdc_cfg);
    if (! usb_cdc)
        return 0;

    usb_cdc_tty = tty_init (&usb_cdc_tty_cfg, usb_cdc);
    if (!usb_cdc_tty)
        return 0;

    tty_echo_set (usb_cdc_tty, cfg->echo);
    
    sys_device_register (cfg->devname, &tty_file_ops, usb_cdc_tty);
    return 1;
}

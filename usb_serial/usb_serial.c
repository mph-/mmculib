#include "usb_serial.h"
#include "usb_cdc.h"
#include "tty.h"
#include "sys.h"
#include <stdlib.h>


usb_serial_t *
usb_serial_init (const usb_serial_cfg_t *cfg, const char *devname)
{
    usb_serial_t *dev;
    tty_cfg_t tty_cfg =
        {
            .read = (void *)usb_cdc_read,
            .write = (void *)usb_cdc_write,
            .linebuffer_size = 80
        };        

    dev = calloc (1, sizeof (*dev));
    if (! dev)
        return 0;

    dev->usb_cdc = usb_cdc_init (cfg);
    if (! dev->usb_cdc)
    {
        free (dev);
        return 0;
    }

    dev->tty = tty_init (&tty_cfg, dev->usb_cdc);
    if (! dev->tty)
    {
        free (dev);
        return 0;
    }

    /* Do not echo by default.  The linux ACM driver creates a new
       instance with echo enabled every time the embedded program
       starts.  The echoing is not turned off until a serial terminal
       applications such as gtkterm is run or using stty -F
       /dev/ttyACM0 -echo.  If we also echo, then we have an echo
       chamber.  */
    
    sys_device_register (devname, &tty_file_ops, dev->tty);
    return dev;
}


void usb_serial_echo_set (usb_serial_t *dev, bool echo)
{
    tty_echo_set (dev->tty, echo);
}


void usb_serial_shutdown (usb_serial_t *dev)
{
    usb_cdc_shutdown ();
}


void usb_serial_puts (usb_serial_t *dev, const char *str)
{
    tty_puts (dev->tty, str);
}


char *usb_serial_gets (usb_serial_t *dev, char *buffer, int size)
{
    return tty_gets (dev->tty, buffer, size);
}

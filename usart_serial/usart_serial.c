#include "usart_serial.h"
#include "busart.h"
#include "tty.h"
#include "sys.h"
#include <stdlib.h>


usart_serial_t *
usart_serial_init (const usart_serial_cfg_t *cfg, const char *devname)
{
    usart_serial_t *dev;
    tty_cfg_t tty_cfg =
        {
            .read = (void *)busart_read,
            .write = (void *)busart_write,
            .linebuffer_size = 80
        };    

    dev = calloc (1, sizeof (*dev));
    if (! dev)
        return 0;

    dev->busart = busart_init (cfg);
    if (! dev->busart)
    {
        free (dev);
        return 0;
    }

    dev->tty = tty_init (&tty_cfg, dev->busart);
    if (! dev->tty)
    {
        free (dev);
        return 0;
    }

    sys_device_register (devname, &tty_file_ops, dev->tty);
    return dev;
}


void usart_serial_echo_set (usart_serial_t *dev, bool echo)
{
    tty_echo_set (dev->tty, echo);
}

void usart_serial_shutdown (usart_serial_t *dev)
{
    // TODO, shutdown appropriate peripheral
}


void usart_serial_puts (usart_serial_t *dev, const char *str)
{
    tty_puts (dev->tty, str);
}


char *usart_serial_gets (usart_serial_t *dev, char *buffer, int size)
{
    return tty_gets (dev->tty, buffer, size);
}

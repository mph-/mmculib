/** @file   buart.c
    @author M. P. Hayes, UCECE
    @date   15 May 2018
    @brief  Buffered UART implementation.  */

#include "errno.h"
#include "ring.h"
#include "buart.h"
#include "peripherals.h"
#include <string.h>
#include <stdlib.h>


/** A BUART can be disabled in the target.h file, e.g., using
    #define BUART0_ENABLE 0.  */

#ifndef BUART0_ENABLE
#define BUART0_ENABLE (UART_NUM >= 1)
#endif

#ifndef BUART1_ENABLE
#define BUART1_ENABLE (UART_NUM >= 2)
#endif

#if !defined(BUART0_ENABLE) && !defined(BUART1_ENABLE)
#error "Neither BUART0_ENABLE or BUART1_ENABLE defined"
#endif


struct buart_dev_struct
{
    void (*tx_irq_enable) (void);
    void (*rx_irq_enable) (void);
    bool (*tx_finished_p) (void);
    ring_t tx_ring;
    ring_t rx_ring;
    uint32_t read_timeout_us;
    uint32_t write_timeout_us;        
};


/* Include machine dependent buart definitions.  */
#if BUART0_ENABLE
#include "buart0_isr.h"
#endif

#if BUART1_ENABLE
#include "buart1_isr.h"
#endif


/** Initialise buffered UART driver
    @param cfg pointer to configuration structure.
    @return pointer to buart device structure.
*/
buart_t 
buart_init (const buart_cfg_t *cfg)
{
    buart_dev_t *dev = 0;
    uint16_t baud_divisor;
    char *tx_buffer;
    char *rx_buffer;

    if (cfg->baud_rate == 0)
        baud_divisor = cfg->baud_divisor;
    else
        baud_divisor = BUART_BAUD_DIVISOR (cfg->baud_rate);

#if BUART0_ENABLE
    if (cfg->channel == 0)
        dev = buart0_init (baud_divisor);
#endif

#if BUART1_ENABLE
    if (cfg->channel == 1)
        dev = buart1_init (baud_divisor);
#endif

    if (!dev)
        return 0;

    dev->read_timeout_us = cfg->read_timeout_us;
    dev->write_timeout_us = cfg->write_timeout_us;
    
    tx_buffer = cfg->tx_buffer;
    rx_buffer = cfg->rx_buffer;

    if (!tx_buffer)
        tx_buffer = malloc (cfg->tx_size);
    if (!tx_buffer)
        return 0;

    if (!rx_buffer)
        rx_buffer = malloc (cfg->rx_size);
    if (!rx_buffer)
    {
        free (tx_buffer);
        return 0;
    }

    ring_init (&dev->tx_ring, tx_buffer, cfg->tx_size);
    
    ring_init (&dev->rx_ring, rx_buffer, cfg->rx_size);

    /* Enable the rx interrupt now.  The tx interrupt is enabled
       when we perform a write.  */
    dev->rx_irq_enable ();

    return dev;
}


/** Write as many bytes (up to the desired size) that can currently
    fit in the ring buffer.  */
static ssize_t
buart_write_nonblock (buart_t buart, const void *data, size_t size)
{
    ssize_t ret;
    buart_dev_t *dev = buart;

    ret = ring_write (&dev->tx_ring, data, size);

    dev->tx_irq_enable ();

    if (ret == 0 && size != 0)
    {
        /* Would block.  */
        errno = EAGAIN;
        return -1;
    }
    return ret;
}


/** Read as many bytes as there are available in the ring buffer up to
    the specifed size.  */
static ssize_t
buart_read_nonblock (buart_t buart, void *data, size_t size)
{
    buart_dev_t *dev = buart;
    ssize_t ret;

    ret = ring_read (&dev->rx_ring, data, size);
    
    if (ret == 0 && size != 0)
    {
        /* Would block.  */
        errno = EAGAIN;
        return -1;
    }
    return ret;
}


/** Read size bytes.  Block until all the bytes have been read or
    until timeout occurs.  */
ssize_t
buart_read (void *buart, void *data, size_t size)
{
    buart_dev_t *dev = buart;
    
    return sys_read_timeout (buart, data, size, dev->read_timeout_us,
                             (void *)buart_read_nonblock);
}


/** Write size bytes.  Block until all the bytes have been transferred
    to the transmit ring buffer or until timeout occurs.  */
ssize_t
buart_write (void *buart, const void *data, size_t size)
{
    buart_dev_t *dev = buart;
    
    return sys_write_timeout (buart, data, size, dev->write_timeout_us,
                              (void *)buart_write_nonblock);
}


/** Return the number of bytes immediately available for reading.  */
ring_size_t
buart_read_num (buart_t buart)
{
    buart_dev_t *dev = buart;

    return ring_read_num (&dev->rx_ring);
}


/** Return the number of bytes immediately available for writing.  */
ring_size_t
buart_write_num (buart_t buart)
{
    buart_dev_t *dev = buart;

    return ring_write_num (&dev->tx_ring);
}


/** Return non-zero if there is a character ready to be read.  */
bool
buart_read_ready_p (buart_t buart)
{
    return buart_read_num (buart);
}


/** Return non-zero if a character can be written without blocking.  */
bool
buart_write_ready_p (buart_t buart)
{
    return buart_write_num (buart);
}


/** Return non-zero if transmitter finished.  */
bool
buart_write_finished_p (buart_t buart)
{
    buart_dev_t *dev = buart;

    return ring_empty_p (&dev->tx_ring)
        && dev->tx_finished_p ();
}


/** Read character.  */
int
buart_getc (buart_t buart)
{
    uint8_t ch = 0;

    if (buart_read (buart, &ch, sizeof (ch)) != sizeof (ch))
        return -1;
    return ch;
}


/** Write character.  */
int
buart_putc (buart_t buart, char ch)
{
    if (ch == '\n')
        buart_putc (buart, '\r');    

    if (buart_write (buart, &ch, sizeof (ch)) != sizeof (ch))
        return -1;
    return ch;
}


/** Write string.  */
int
buart_puts (buart_t buart, const char *str)
{
    while (*str)
        if (buart_putc (buart, *str++) < 0)
            return -1;
    return 1;
}


/** Clears the buart's transmit and receive buffers.  */
void
buart_clear (buart_t buart)
{
    buart_dev_t *dev = buart;

    ring_clear (&dev->rx_ring);
    ring_clear (&dev->tx_ring);
}


const sys_file_ops_t buart_file_ops =
{
    .read = buart_read,
    .write = buart_write
};

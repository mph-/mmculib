/** @file   busart.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  Buffered USART implementation.  */

#include "errno.h"
#include "ring.h"
#include "busart.h"
#include "peripherals.h"
#include <string.h>
#include <stdlib.h>


/** A BUSART can be disabled in the target.h file, e.g., using
    #define BUSART0_ENABLE 0.  */

#ifndef BUSART0_ENABLE
#define BUSART0_ENABLE (USART_NUM >= 1)
#endif

#ifndef BUSART1_ENABLE
#define BUSART1_ENABLE (USART_NUM >= 2)
#endif

#if !defined(BUSART0_ENABLE) && !defined(BUSART1_ENABLE)
#error "Neither BUSART0_ENABLE or BUSART1_ENABLE defined"
#endif


#ifndef BUSART_LINE_BUFFER_SIZE
#define BUSART_LINE_BUFFER_SIZE 82
#endif


#ifndef BUSART_SPRINTF_BUFFER_SIZE
#define BUSART_SPRINTF_BUFFER_SIZE 128
#endif



struct busart_dev_struct
{
    void (*tx_irq_enable) (void);
    void (*rx_irq_enable) (void);
    bool (*tx_finished_p) (void);
    ring_t tx_ring;
    ring_t rx_ring;
    uint32_t read_timeout_us;
    uint32_t write_timeout_us;
};


/* Include machine dependent busart definitions.  */
#if BUSART0_ENABLE
#include "busart0_isr.h"
#endif

#if BUSART1_ENABLE
#include "busart1_isr.h"
#endif


/* How should hardware flow-control be implemented?  The easiest and
   most general approach is to place the responsibility on the user
   and have a function to pause/resume data transmission.  This might
   not be as fine-grained as having the TX ISR check.  This would also
   require that writes are non-blocking otherwise the user does not
   get a chance to monitor the flow-control signals.

   What about half-duplex operation, say for IRDA?  Do we really need
   to disable the receiver when transmitting?  With the receiver
   enabled we will pick up what we are sending, in theory.  For some
   applications it is necessary to disable the transmitter buffer to
   avoid bus conflict.  While most processors have an interrupt when the
   transmitter finishes, for generality, it is easier to poll.
*/


/** Initialise buffered USART driver
    @param cfg pointer to configuration structure.
    @return pointer to busart device structure.
*/
busart_t
busart_init (const busart_cfg_t *cfg)
{
    busart_dev_t *dev = 0;
    uint16_t baud_divisor;
    ring_size_t size;
    char *tx_buffer;
    char *rx_buffer;

    if (cfg->baud_rate == 0)
        baud_divisor = cfg->baud_divisor;
    else
        baud_divisor = BUSART_BAUD_DIVISOR (cfg->baud_rate);

#if BUSART0_ENABLE
    if (cfg->channel == 0)
        dev = busart0_init (baud_divisor);
#endif

#if BUSART1_ENABLE
    if (cfg->channel == 1)
        dev = busart1_init (baud_divisor);
#endif

    if (!dev)
        return 0;

    dev->read_timeout_us = cfg->read_timeout_us;
    dev->write_timeout_us = cfg->write_timeout_us;

    tx_buffer = cfg->tx_buffer;
    rx_buffer = cfg->rx_buffer;

    if (!tx_buffer)
    {
        size = cfg->tx_size;
        if (size == 0)
            size = 64;
        tx_buffer = malloc (size);
    }
    if (!tx_buffer)
        return 0;

    if (!rx_buffer)
    {
        size = cfg->rx_size;
        if (size == 0)
            size = 64;
        rx_buffer = malloc (size);
    }
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
busart_write_nonblock (busart_t busart, const void *data, size_t size)
{
    ssize_t ret;
    busart_dev_t *dev = busart;

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
busart_read_nonblock (busart_t busart, void *data, size_t size)
{
    busart_dev_t *dev = busart;
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
busart_read (void *busart, void *data, size_t size)
{
    busart_dev_t *dev = busart;

    return sys_read_timeout (busart, data, size, dev->read_timeout_us,
                             (void *)busart_read_nonblock);
}


/** Write size bytes.  Block until all the bytes have been transferred
    to the transmit ring buffer or until timeout occurs.  */
ssize_t
busart_write (void *busart, const void *data, size_t size)
{
    busart_dev_t *dev = busart;

    return sys_write_timeout (busart, data, size, dev->write_timeout_us,
                              (void *)busart_write_nonblock);
}


/** Return the number of bytes immediately available for reading.  */
ring_size_t
busart_read_num (busart_t busart)
{
    busart_dev_t *dev = busart;

    return ring_read_num (&dev->rx_ring);
}


/** Return the number of bytes immediately available for writing.  */
ring_size_t
busart_write_num (busart_t busart)
{
    busart_dev_t *dev = busart;

    return ring_write_num (&dev->tx_ring);
}


/** Return non-zero if there is a character ready to be read.  */
bool
busart_read_ready_p (busart_t busart)
{
    return busart_read_num (busart);
}


/** Return non-zero if a character can be written without blocking.  */
bool
busart_write_ready_p (busart_t busart)
{
    return busart_write_num (busart);
}


/** Return non-zero if transmitter finished.  */
bool
busart_write_finished_p (busart_t busart)
{
    busart_dev_t *dev = busart;

    return ring_empty_p (&dev->tx_ring)
        && dev->tx_finished_p ();
}


/** Read character.  */
int
busart_getc (busart_t busart)
{
    uint8_t ch = 0;

    if (busart_read (busart, &ch, sizeof (ch)) != sizeof (ch))
        return -1;
    return ch;
}


/** Write character.  */
int
busart_putc (busart_t busart, char ch)
{
    if (ch == '\n')
        busart_putc (busart, '\r');

    if (busart_write (busart, &ch, sizeof (ch)) != sizeof (ch))
        return -1;
    return ch;
}


/** Write string.  */
int
busart_puts (busart_t busart, const char *str)
{
    while (*str)
        if (busart_putc (busart, *str++) < 0)
            return -1;
    return 1;
}


/* Non-blocking equivalent to fgets.  Returns 0 if a line is not available
   other pointer to buffer.  */
char *
busart_gets (busart_t busart, char *buffer, int size)
{
    static char line_buffer[BUSART_LINE_BUFFER_SIZE] = "";
    static int count = 0;
    int c;
    int i;

    while (1)
    {
        c = busart_getc (busart);
        if (c == -1)
            return 0;

        line_buffer[count] = c;
        count++;
        if (c == '\n' || count >= size)
            break;
    }

    if (size > count)
        size = count;

    for (i = 0; i < size; i++)
        buffer[i] = line_buffer[i];
    buffer[i] = 0;

    /* Could use ring buffer to avoid copying.  */
    for (i = count - size - 1; i >= 0; i--)
        line_buffer[i] = line_buffer[i + count];
    count -= size;

    return buffer;
}


int
busart_printf (busart_t busart, const char *fmt, ...)
{
    // FIXME, long strings can overflow the buffer and thus be truncated.
    char buffer[BUSART_SPRINTF_BUFFER_SIZE];
    va_list ap;
    int ret;

    va_start (ap, fmt);
    ret = vsnprintf (buffer, sizeof (buffer), fmt, ap);
    va_end (ap);

    busart_puts (busart, buffer);
    return ret;
}


/** Clears the busart's transmit and receive buffers.  */
void
busart_clear (busart_t busart)
{
    busart_dev_t *dev = busart;

    ring_clear (&dev->rx_ring);
    ring_clear (&dev->tx_ring);
}


const sys_file_ops_t busart_file_ops =
{
    .read = busart_read,
    .write = busart_write
};

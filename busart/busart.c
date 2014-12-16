/** @file   busart.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  Buffered USART implementation.  */

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


struct busart_dev_struct
{
    void (*tx_irq_enable) (void);
    void (*rx_irq_enable) (void);
    bool (*tx_finished_p) (void);
    ring_t tx_ring;
    ring_t rx_ring;
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


busart_t 
busart_init (uint8_t channel,
             uint16_t baud_divisor,
             char *tx_buffer, ring_size_t tx_size,
             char *rx_buffer, ring_size_t rx_size)
{
    busart_dev_t *dev = 0;

#if BUSART0_ENABLE
    if (channel == 0)
        dev = busart0_init (baud_divisor);
#endif

#if BUSART1_ENABLE
    if (channel == 1)
        dev = busart1_init (baud_divisor);
#endif

    if (!dev)
        return 0;

    if (!tx_buffer)
        tx_buffer = malloc (tx_size);
    if (!tx_buffer)
        return 0;

    if (!rx_buffer)
        rx_buffer = malloc (rx_size);
    if (!rx_buffer)
    {
        free (tx_buffer);
        return 0;
    }

    ring_init (&dev->tx_ring, tx_buffer, tx_size);
    
    ring_init (&dev->rx_ring, rx_buffer, rx_size);

    /* Enable the rx interrupt now.  The tx interrupt is enabled
       when we perform a write.  */
    dev->rx_irq_enable ();

    return dev;
}


/** Write size bytes.  Currently this only writes as many bytes (up to
    the desired size) that can currently fit in the ring buffer.   */
ring_size_t
busart_write (busart_t busart, const void *data, ring_size_t size)
{
    int ret;
    busart_dev_t *dev = busart;

    ret = ring_write (&dev->tx_ring, data, size);

    dev->tx_irq_enable ();

    return ret;
}


/** Write size bytes.  This will block until the desired number of
    bytes have been written.  */
ring_size_t
busart_write_block (busart_t busart, const void *data, ring_size_t size)
{
    ring_size_t left = size;
    const char *buffer = data;

    while (left)
    {
        ring_size_t ret;

        ret = busart_write (busart, buffer, left);
        buffer += ret;
        left -= ret;
    }
    return size;
}


/** Read as many bytes as there are available in the ring buffer up to
    the specifed size.  */
ring_size_t
busart_read (busart_t busart, void *data, ring_size_t size)
{
    busart_dev_t *dev = busart;

    return ring_read (&dev->rx_ring, data, size);
}


/** Read size bytes.  This will block until the desired number of
    bytes have been read.  */
ring_size_t
busart_read_block (busart_t busart, void *data, ring_size_t size)
{
    ring_size_t left = size;
    char *buffer = data;

    while (left)
    {
        ring_size_t ret;

        ret = busart_read (busart, buffer, left);
        buffer += ret;
        left -= ret;
    }
    return size;
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


/** Read character.  This blocks until the character can be read.  */
int8_t
busart_getc (busart_t busart)
{
    uint8_t ch;

    busart_read_block (busart, &ch, sizeof (ch));
    return ch;
}


/** Write character.  This blocks until the character can be
    written.  */
int8_t
busart_putc (busart_t busart, char ch)
{
    if (ch == '\n')
        busart_putc (busart, '\r');    

    busart_write_block (busart, &ch, sizeof (ch));
    return ch;
}


/** Write string.  This blocks until the string is buffered.  */
int8_t
busart_puts (busart_t busart, const char *str)
{
    while (*str)
        busart_putc (busart, *str++);
    return 1;
}

/** @file   lusart.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  Line buffered USART implementation.  */

#include "errno.h"
#include "lusart.h"
#include "peripherals.h"
#include <string.h>
#include <stdlib.h>


/** A LUSART can be disabled in the target.h file, e.g., using
    #define LUSART0_ENABLE 0.  */

#ifndef LUSART0_ENABLE
#define LUSART0_ENABLE (USART_NUM >= 1)
#endif

#ifndef LUSART1_ENABLE
#define LUSART1_ENABLE (USART_NUM >= 2)
#endif

#if !defined(LUSART0_ENABLE) && !defined(LUSART1_ENABLE)
#error "Neither LUSART0_ENABLE or LUSART1_ENABLE defined"
#endif


#ifndef LUSART_LINE_BUFFER_SIZE
#define LUSART_LINE_BUFFER_SIZE 82
#endif


#ifndef LUSART_SPRINTF_BUFFER_SIZE
#define LUSART_SPRINTF_BUFFER_SIZE 128
#endif



struct lusart_dev_struct
{
    void (*tx_irq_enable) (void);
    void (*rx_irq_enable) (void);
    bool (*tx_finished_p) (void);
    uint32_t read_timeout_us;
    uint32_t write_timeout_us;
    char *tx_buffer;
    char *rx_buffer;
    uint16_t tx_size;
    uint16_t tx_in;
    volatile uint16_t tx_out;
    uint16_t rx_size;
    volatile uint16_t rx_in;
    uint16_t rx_out;
    volatile uint8_t rx_nl_in;
    uint8_t rx_nl_out;
};


/* Include machine dependent lusart definitions.  */
#if LUSART0_ENABLE
#include "lusart0_isr.h"
#endif

#if LUSART1_ENABLE
#include "lusart1_isr.h"
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
   avoid lus conflict.  While most processors have an interrupt when the
   transmitter finishes, for generality, it is easier to poll.
*/


/** Initialise buffered USART driver
    @param cfg pointer to configuration structure.
    @return pointer to lusart device structure.
*/
lusart_t
lusart_init (const lusart_cfg_t *cfg)
{
    lusart_dev_t *dev = 0;
    uint16_t baud_divisor;
    uint16_t rx_size;
    uint16_t tx_size;
    char *tx_buffer;
    char *rx_buffer;

    if (cfg->baud_rate == 0)
        baud_divisor = cfg->baud_divisor;
    else
        baud_divisor = LUSART_BAUD_DIVISOR (cfg->baud_rate);

#if LUSART0_ENABLE
    if (cfg->channel == 0)
        dev = lusart0_init (baud_divisor);
#endif

#if LUSART1_ENABLE
    if (cfg->channel == 1)
        dev = lusart1_init (baud_divisor);
#endif

    if (!dev)
        return 0;

    dev->read_timeout_us = cfg->read_timeout_us;
    dev->write_timeout_us = cfg->write_timeout_us;

    tx_buffer = cfg->tx_buffer;
    rx_buffer = cfg->rx_buffer;

    tx_size = cfg->tx_size;
    if (!tx_buffer)
    {
        if (tx_size == 0)
            tx_size = 64;
        tx_buffer = malloc (tx_size);
    }
    if (!tx_buffer)
        return 0;

    dev->tx_buffer = tx_buffer;
    dev->tx_size = tx_size;
    dev->tx_in = dev->tx_out = 0;

    rx_size = cfg->rx_size;
    if (!rx_buffer)
    {
        if (rx_size == 0)
            rx_size = 64;
        rx_buffer = malloc (rx_size);
    }
    if (!rx_buffer)
    {
        free (tx_buffer);
        return 0;
    }

    dev->rx_buffer = rx_buffer;
    dev->rx_size = rx_size;
    dev->rx_in = dev->rx_out = 0;
    dev->rx_nl_in = dev->rx_nl_out = 0;

    /* Enable the rx interrupt now.  The tx interrupt is enabled
       when we perform a write.  */
    dev->rx_irq_enable ();

    return dev;
}


/** Return non-zero if transmitter finished.  */
bool
lusart_write_finished_p (lusart_t lusart)
{
    lusart_dev_t *dev = lusart;

    return dev->tx_in == dev->tx_out && dev->tx_finished_p ();
}


/** Read character (non-blocking).  Returns -1 if nothing to read.  */
int
lusart_getc (lusart_t lusart)
{
    lusart_dev_t *dev = lusart;
    char ch;

    if (dev->rx_in == dev->rx_out)
        return -1;

    ch = dev->rx_buffer[dev->rx_out];
    dev->rx_out++;
    if (dev->rx_out >= dev->rx_size)
        dev->rx_out = 0;

    if (ch == '\n')
        dev->rx_nl_out++;

    return ch;
}


/** Write character.  */
int
lusart_putc (lusart_t lusart, char ch)
{
    lusart_dev_t *dev = lusart;

    if (0 && ch == '\n')
        lusart_putc (lusart, '\r');

    // What if the buffer is full?

    dev->tx_buffer[dev->tx_in] = ch;
    dev->tx_in++;
    if (dev->tx_in >= dev->tx_size)
        dev->tx_in = 0;

    dev->tx_irq_enable ();
    return ch;
}


/** Write string.  */
int
lusart_puts (lusart_t lusart, const char *str)
{
    while (*str)
        if (lusart_putc (lusart, *str++) < 0)
            return -1;
    return 1;
}


/* Non-blocking equivalent to fgets.  Returns 0 if a line is not
   available otherwise return pointer to buffer.  */
char *
lusart_gets (lusart_t lusart, char *buffer, int size)
{
    lusart_dev_t *dev = lusart;
    int i;

    if (dev->rx_nl_in == dev->rx_nl_out)
        return 0;

    for (i = 0; i < size; i++)
    {
        char ch;

        ch = lusart_getc (dev);

        buffer[i] = ch;
        if (ch == '\n')
        {
            i++;
            break;
        }
    }
    buffer[i] = 0;

    return buffer;
}


int
lusart_printf (lusart_t lusart, const char *fmt, ...)
{
    // FIXME, long strings can overflow the buffer and thus be truncated.
    char buffer[LUSART_SPRINTF_BUFFER_SIZE];
    va_list ap;
    int ret;

    va_start (ap, fmt);
    ret = vsnprintf (buffer, sizeof (buffer), fmt, ap);
    va_end (ap);

    lusart_puts (lusart, buffer);
    return ret;
}


/** Clears the lusart's transmit and receive buffers.  */
void
lusart_clear (lusart_t lusart)
{
    lusart_dev_t *dev = lusart;

    dev->tx_in = dev->tx_out = 0;
    dev->rx_in = dev->rx_out = 0;
    dev->rx_nl_in = dev->rx_nl_out = 0;
}

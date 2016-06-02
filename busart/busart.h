/** @file   busart.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  Buffered USART interface.
    @note By default hardware handshaking is not used for flow
    control.  This can be enabled by defining USART0_USE_HANDSHAKING
    or USART1_USE_HANDSHAKING in target.h
*/
#ifndef BUSART_H
#define BUSART_H

#include "config.h"
#include "sys.h"
#include "ring.h"

#include "usart0.h"

typedef struct busart_dev_struct busart_dev_t;

typedef busart_dev_t *busart_t;

#define BUSART_BAUD_DIVISOR(BAUD_RATE) USART0_BAUD_DIVISOR(BAUD_RATE)


/** Busart configuration structure.  */
typedef struct
{
    /* 0 for USART0, 1 for USART1.  */
    uint8_t channel;
    /* Baud rate.  */
    uint32_t baud_rate;
    /* Baud rate divisor (this is used if baud_rate is zero).  */
    uint32_t baud_divisor;
    /* Buffer used for the transmit ring buffer (if zero one is
       allocated with malloc).  */
    char *tx_buffer;
    /* Buffer used for the receive ring buffer (if zero one is
       allocated with malloc).  */
    char *rx_buffer; 
    /* Size of the transmit ring buffer in bytes.  */
    ring_size_t tx_size;
    /* Size of the receive ring buffer in bytes.  */
    ring_size_t rx_size;
}
busart_cfg_t;



/** Initialise buffered USART driver
    @param cfg pointer to configuration structure.
    @return pointer to busart device structure.
*/
busart_t
busart_init (const busart_cfg_t *cfg);


/* Read as many bytes as there are available in the ring buffer up to
   the specifed size.  */
ssize_t
busart_read (busart_t busart, void *data, size_t size);


/* Read size bytes.  This will block until the desired number of
   bytes have been read.  */
ssize_t
busart_read_block (busart_t busart, void *data, size_t size);


/* Write size bytes.  Currently this only writes as many bytes (up to
   the desired size) that can currently fit in the ring buffer.   */
ssize_t
busart_write (busart_t busart, const void *data, size_t size);


/* Write size bytes.  This will block until the desired number of
   bytes have been written.  */
ssize_t
busart_write_block (busart_t busart, const void *data, size_t size);


/* Return the number of bytes immediately available for reading.  */
ring_size_t
busart_read_num (busart_t busart);


/* Return the number of bytes immediately available for writing.  */
ring_size_t
busart_write_num (busart_t busart);


/* Return non-zero if there is a character ready to be read.  */
bool
busart_read_ready_p (busart_t busart);


/* Return non-zero if a character can be written without blocking.  */
bool
busart_write_ready_p (busart_t busart);


/* Return non-zero if transmitter finished.  */
bool
busart_write_finished_p (busart_t busart);


/* Read character.  */
int
busart_getc (busart_t busart);


/* Write character.  */
int
busart_putc (busart_t busart, char ch);


/* Write string.  This blocks until the string is buffered.  */
int
busart_puts (busart_t busart, const char *str);


/* Clears the busart's transmit and receive buffers.  */
extern void
busart_clear (busart_t busart);
#endif

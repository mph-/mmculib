/** @file   busart.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  Buffered USART interface.
*/
#ifndef BUSART_H
#define BUSART_H

#include "config.h"
#include "ring.h"

#include "usart0.h"

typedef struct busart_dev_struct busart_dev_t;

typedef busart_dev_t *busart_t;

#define BUSART_BAUD_DIVISOR(BAUD_RATE) USART0_BAUD_DIVISOR(BAUD_RATE)


extern busart_t
busart_init (uint8_t channel,
             uint16_t baud_divisor,
             char *tx_buffer, ring_size_t tx_size,
             char *rx_buffer, ring_size_t rx_size);


/* Read as many bytes as there are available in the ring buffer up to
   the specifed size.  */
extern ring_size_t
busart_read (busart_t busart, void *data, ring_size_t size);


/* Read size bytes.  This will block until the desired number of
   bytes have been read.  */
extern ring_size_t
busart_read_block (busart_t busart, void *data, ring_size_t size);


/* Write size bytes.  Currently this only writes as many bytes (up to
   the desired size) that can currently fit in the ring buffer.   */
extern ring_size_t
busart_write (busart_t busart, const void *data, ring_size_t size);


/* Write size bytes.  This will block until the desired number of
   bytes have been written.  */
extern ring_size_t
busart_write_block (busart_t busart, const void *data, ring_size_t size);


/* Return the number of bytes immediately available for reading.  */
extern ring_size_t
busart_read_num (busart_t busart);


/* Return the number of bytes immediately available for writing.  */
extern ring_size_t
busart_write_num (busart_t busart);


/* Return non-zero if there is a character ready to be read.  */
extern bool
busart_read_ready_p (busart_t busart);


/* Return non-zero if a character can be written without blocking.  */
extern bool
busart_write_ready_p (busart_t busart);


/* Return non-zero if transmitter finished.  */
extern bool
busart_write_finished_p (busart_t busart);


/* Read character.  */
extern int8_t
busart_getc (busart_t busart);


/* Write character.  */
extern int8_t
busart_putc (busart_t busart, char ch);


/* Write string.  This blocks until the string is buffered.  */
extern int8_t
busart_puts (busart_t busart, const char *str);

#endif

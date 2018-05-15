/** @file   buart.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  Buffered UART interface.
    @note By default hardware handshaking is not used for flow
    control.  This can be enabled by defining UART0_USE_HANDSHAKING
    or UART1_USE_HANDSHAKING in target.h
*/
#ifndef BUART_H
#define BUART_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "sys.h"
#include "ring.h"

#include "uart0.h"

typedef struct buart_dev_struct buart_dev_t;

typedef buart_dev_t *buart_t;

#define BUART_BAUD_DIVISOR(BAUD_RATE) UART0_BAUD_DIVISOR(BAUD_RATE)


/** Buart configuration structure.  */
typedef struct
{
    /* 0 for UART0, 1 for UART1.  */
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
    int read_timeout_us;
    int write_timeout_us;
}
buart_cfg_t;



/** Initialise buffered UART driver
    @param cfg pointer to configuration structure.
    @return pointer to buart device structure.
*/
buart_t
buart_init (const buart_cfg_t *cfg);


/** Read size bytes.  Block until all the bytes have been read or
    until timeout occurs.  */
ssize_t
buart_read (void *buart, void *data, size_t size);


/** Write size bytes.  Block until all the bytes have been transferred
    to the transmit ring buffer or until timeout occurs.  */
ssize_t
buart_write (void *buart, const void *data, size_t size);


/* Return the number of bytes immediately available for reading.  */
ring_size_t
buart_read_num (buart_t buart);


/* Return the number of bytes immediately available for writing.  */
ring_size_t
buart_write_num (buart_t buart);


/* Return non-zero if there is a character ready to be read.  */
bool
buart_read_ready_p (buart_t buart);


/* Return non-zero if a character can be written without blocking.  */
bool
buart_write_ready_p (buart_t buart);


/* Return non-zero if transmitter finished.  */
bool
buart_write_finished_p (buart_t buart);


/* Read character.  */
int
buart_getc (buart_t buart);


/* Write character.  */
int
buart_putc (buart_t buart, char ch);


/* Write string.  This blocks until the string is buffered.  */
int
buart_puts (buart_t buart, const char *str);


/* Clears the buart's transmit and receive buffers.  */
void
buart_clear (buart_t buart);

extern const sys_file_ops_t buart_file_ops;


#ifdef __cplusplus
}
#endif    
#endif


/** @file   lusart.h
    @author M. P. Hayes, UCECE
    @date   5 Jan 2024
    @brief  Line buffered USART interface.
    @note By default hardware handshaking is not used for flow
    control.  This can be enabled by defining USART0_USE_HANDSHAKING
    or USART1_USE_HANDSHAKING in target.h
*/
#ifndef LUSART_H
#define LUSART_H

#ifdef __cplusplus
extern "C" {
#endif


#include "config.h"
#include "sys.h"
#include "usart0.h"
#include <stdarg.h>


typedef struct lusart_dev_struct lusart_dev_t;

typedef lusart_dev_t *lusart_t;

#define LUSART_BAUD_DIVISOR(BAUD_RATE) USART0_BAUD_DIVISOR(BAUD_RATE)


/** Lusart configuration structure.  */
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
    /* Size of the transmit ring buffer in bytes (default 64).  */
    uint16_t tx_size;
    /* Size of the receive ring buffer in bytes (default 64).  */
    uint16_t rx_size;
    int read_timeout_us;
    int write_timeout_us;
}
lusart_cfg_t;



/** Initialise buffered USART driver
    @param cfg pointer to configuration structure.
    @return pointer to lusart device structure.
*/
lusart_t
lusart_init (const lusart_cfg_t *cfg);


/** Read character (non-blocking).  Returns -1 if nothing to read.  */
int
lusart_getc (lusart_t lusart);


/* Write character.  */
int
lusart_putc (lusart_t lusart, char ch);


/* Write string.  This blocks until the string is buffered.  */
int
lusart_puts (lusart_t lusart, const char *str);


/* Non-blocking equivalent to fgets.  Returns 0 if a line is not available
   other pointer to buffer.  */
char *
lusart_gets (lusart_t lusart, char *buffer, int size);


int
lusart_printf (lusart_t lusart, const char *fmt, ...);


/* Return non-zero if transmitter finished.  */
bool
lusart_write_finished_p (lusart_t lusart);


/* Clears the lusart's transmit and receive buffers.  */
void
lusart_clear (lusart_t lusart);


#ifdef __cplusplus
}
#endif
#endif

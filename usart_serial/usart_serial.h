#ifndef USART_SERIAL_H
#define USART_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif


#include "config.h"
#include "busart.h"
#include "tty.h"

/** usart_serial configuration structure.  */
typedef busart_cfg_t usart_serial_cfg_t;


typedef struct
{
    busart_t busart;
    tty_t *tty;
} usart_serial_t;


int
usart_serial_stdio_init (const usart_serial_cfg_t *cfg);


usart_serial_t *
usart_serial_init (const usart_serial_cfg_t *cfg, const char *devname);


void
usart_serial_shutdown (usart_serial_t *dev);


void
usart_serial_echo_set (usart_serial_t *dev, bool echo);


void
usart_serial_puts (usart_serial_t *dev, const char *str);


char *
usart_serial_gets (usart_serial_t *dev, char *buffer, int size);


int
usart_serial_getc (usart_serial_t *dev);


#ifdef __cplusplus
}
#endif
#endif

/** @file   tty.h
    @author M. P. Hayes
    @date   12 May 2016
    @brief  A non-blocking TTY driver.  It does not support termio.  
*/

#ifndef TTY_H
#define TTY_H

#include "config.h"
#include "sys.h"


/* The longest input linebuffer size.  */
#ifndef TTY_INPUT_BUFFER_SIZE
#define TTY_INPUT_BUFFER_SIZE 80
#endif


/* The longest line for tty_printf.  */
#ifndef TTY_OUTPUT_BUFFER_SIZE
#define TTY_OUTPUT_BUFFER_SIZE 1024
#endif


struct tty_cfg_struct
{
    bool (*update)(void);
    void (*shutdown)(void);
    sys_read_t read;
    sys_write_t write;
};


typedef struct tty_struct tty_t;

typedef struct tty_cfg_struct tty_cfg_t;


int
tty_getc (tty_t *tty);

int
tty_putc (tty_t *tty, int ch);


char *
tty_gets (tty_t *tty, char *buffer, int size);


int
tty_puts (tty_t *tty, const char *s);


int
tty_printf (tty_t *tty, const char *fmt, ...);


bool 
tty_poll (tty_t *tty);


/** Read size bytes.  This will block until the desired number of
    bytes have been read.  */
int16_t
tty_read (tty_t *tty, void *data, uint16_t size);


/** Write size bytes.  This will block until the desired number of
    bytes have been transmitted.  */
int16_t
tty_write (tty_t *tty, const void *data, uint16_t size);


tty_t *
tty_init (const tty_cfg_t *cfg, void *dev);


void
tty_shutdown (tty_t *tty);

#endif

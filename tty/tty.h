/** @file   tty.h
    @author M. P. Hayes
    @date   12 May 2016
    @brief  A non-blocking TTY driver.  It does not support termio.  
    @note The TTY device is a stream oriented device that handles line
          buffering mode.  In this mode, strings are not passed to the
          application until a line has been received.  The TTY driver
          converts incoming carriage returns (usually sent when the Enter key
          is pressed) to newlines.  When sending a newline it also prepends a
          carriage return character.
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
    uint16_t linebuffer_size;
};


typedef struct tty_struct tty_t;

typedef struct tty_cfg_struct tty_cfg_t;


/** This is a non-blocking version of fgetc.  
    @param tty a pointer to the tty device
    @return next character from line buffer if it contains a newline 
            otherwise -1.
*/
int
tty_getc (tty_t *tty);


int
tty_putc (tty_t *tty, int ch);


/** This is a non-blocking version of fgets.   If the tty linebuffer
    contains a newline, then the linebuffer is copied into the user's
    buffer up to and including the newline, provided the user's buffer
    is large enough.
    @param tty a pointer to the tty device
    @param buffer is a pointer to a buffer
    @param size is the number of bytes in the buffer
    @return buffer if a line from the linebuffer has been copied otherwise 0.
*/
char *
tty_gets (tty_t *tty, char *buffer, int size);


int
tty_puts (tty_t *tty, const char *s);


int
tty_printf (tty_t *tty, const char *fmt, ...);


/** Read characters (if any) from the input stream and store in the
    linebuffer.  */
bool 
tty_poll (tty_t *tty);


/** Read size bytes.  */
int16_t
tty_read (tty_t *tty, void *data, uint16_t size);


/** Write size bytes.  */
int16_t
tty_write (tty_t *tty, const void *data, uint16_t size);


tty_t *
tty_init (const tty_cfg_t *cfg, void *dev);


void
tty_shutdown (tty_t *tty);

#endif

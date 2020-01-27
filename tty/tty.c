/** @file   tty.c
    @author M. P. Hayes, UCECE
    @date   24 January 2008
    @brief  TTY driver.  It does not support termio.  
    @note The TTY device is a stream oriented device that handles line
          buffering mode.  In this mode, strings are not passed to the
          application until a line has been received.  The TTY driver
          converts incoming carriage returns (usually sent when the Enter key
          is pressed) to newlines.  When sending a newline it also prepends a
          carriage return character.
*/

#include "config.h"
#include "linebuffer.h"
#include "tty.h"
#include "sys.h"
#include "errno.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


struct tty_struct
{
    linebuffer_t *linebuffer;
    void *dev;
    sys_read_t read;
    sys_write_t write;
    bool (*update)(void);
    void (*shutdown)(void);
    // TODO, replace with bitmasks
    bool echo;
    bool onlcr;                 /* Translate NL -> CR, NL */
    bool icrnl;                 /* Translate CR -> NL */
};


static int
tty_getc1 (tty_t *tty)
{
    char ch = 0;
    int ret;

    ret = tty->read (tty->dev, &ch, 1);

    if (ret == 1)
        return ch;
    return ret;
}


static int
tty_putc1 (tty_t *tty, int ch)
{
    int ret;    

    ret = tty->write (tty->dev, &ch, 1);

    if (ret < 0)
        return -1;
    
    return ch;
}


int
tty_putc (tty_t *tty, int ch)
{
    /* Convert newline to carriage return/line feed.  */
    if (tty->onlcr && ch ==  '\n')
        tty_putc1 (tty, '\r');

    return tty_putc1 (tty, ch);
}


/** String write.  */
int
tty_puts (tty_t *tty, const char *s)
{
    int ret;

    while (*s)
    {
        ret = tty_putc (tty, *s++);

        if (ret < 0)
            return ret;
    }
    return 1;
}


/** Read characters (if any) from input stream and store in
    the linebuffer.  */
bool
tty_poll (tty_t *tty)
{
    int ch;

    while (1)
    {
        if (tty->update && !tty->update ())
            return 0;

        ch = tty_getc1 (tty);
        if (ch < 0)
            return 1;

        if (tty->icrnl && ch == '\r')
            ch = '\n';

        /* Echo character.  */
        if (tty->echo)
            tty_putc (tty, ch);
        
        linebuffer_add (tty->linebuffer, ch);
    }
}


/** tty version of fgetc.  
    @param tty a pointer to the tty device
    @return next character from line buffer otherwise -1 if no character
            is available.
*/
int
tty_getc (tty_t *tty)
{
    int ret;        

    tty_poll (tty);        
    ret = linebuffer_getc (tty->linebuffer);
    return ret;
}


/** tty version of fgets.   If the tty linebuffer
    contains a newline, then the linebuffer is copied into the user's
    buffer up to and including the newline, provided the user's buffer
    is large enough.
    @param tty a pointer to the tty device
    @param buffer is a pointer to a buffer
    @param size is the number of bytes in the buffer
    @return buffer if a line from the linebuffer has been copied otherwise 0. 
*/
char *
tty_gets (tty_t *tty, char *buffer, int size)
{
    ssize_t ret;

    buffer[0] = 0;
    ret = tty_read (tty, buffer, size - 1);
    if (ret < 0)
        return 0;
    buffer[ret] = 0;
    return buffer;
}


int
tty_printf (tty_t *tty, const char *fmt, ...)
{
    va_list ap;
    int ret;
    int len;
    char buffer[TTY_OUTPUT_BUFFER_SIZE];

    /* FIXME for buffer overrun.  Currently, the output will be
       truncated to a length given by TTY_OUTPUT_BUFFER_SIZE.  */
    
    va_start (ap, fmt);
    ret = vsnprintf (buffer, sizeof (buffer), fmt, ap);
    va_end (ap);

    if (tty_puts (tty, buffer) < 0)
        return -1;

    return ret;
}


/** Read size bytes.  */
ssize_t
tty_read (void *tty, void *data, size_t size)
{
    uint16_t count = 0;
    char *buffer = data;

    for (count = 0; count < size - 1; count++)
    {
        int ch;

        ch = tty_getc (tty);
        if (ch < 0)
        {
            if (count == 0 && errno == EAGAIN)
                return -1;
            return count;
        }
        *buffer++ = ch;
        if (ch == '\n')
            return count + 1;
    }
    return count;
}


/** Write size bytes.  */
ssize_t
tty_write (void *tty, const void *data, size_t size)
{
    uint16_t count = 0;
    const char *buffer = data;

    for (count = 0; count < size; count++)
    {
        int ret;

        ret = tty_putc (tty, *buffer++);
        if (ret < 0)
        {
            if (count == 0 && errno == EAGAIN)
                return -1;
            return count;
        }
    }
    return size;
}


void
tty_shutdown (tty_t *tty)
{
    if (tty->shutdown)
        tty->shutdown ();
}


void
tty_echo_set (tty_t *tty, bool echo)
{
    tty->echo = echo;
}


void
tty_onlcr_set (tty_t *tty, bool onlcr)
{
    tty->onlcr = onlcr;
}


void
tty_icrnl_set (tty_t *tty, bool icrnl)
{
    tty->icrnl = icrnl;
}


tty_t *
tty_init (const tty_cfg_t *cfg, void *dev)
{
    tty_t *tty;
    uint16_t linebuffer_size;

    tty = malloc (sizeof (*tty));
    if (!tty)
        return 0;

    tty->dev = dev;

    tty->read = cfg->read;
    tty->write = cfg->write;
    tty->update = cfg->update;
    tty->shutdown = cfg->shutdown;
    /* Do not echo by default.  The linux ACM driver enables echo by
      default; this is then turned off by serial terminal applications
      such as gtkterm.  In the interim, we have an echo chamber.  */
    tty_echo_set (tty, 0);

    tty_onlcr_set (tty, 1);
    tty_icrnl_set (tty, 1);    

    linebuffer_size = cfg->linebuffer_size;
    if (! linebuffer_size)
        linebuffer_size = TTY_INPUT_BUFFER_SIZE;

    tty->linebuffer = linebuffer_init (linebuffer_size);

    return tty;
}


const sys_file_ops_t tty_file_ops =
{
    .read = (void *)tty_read,
    .write = (void *)tty_write,
};


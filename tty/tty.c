/** @file   tty.c
    @author M. P. Hayes, UCECE
    @date   24 January 2008
    @brief  A non-blocking TTY driver.  It does not support termio.  
*/

#include "config.h"
#include "linebuffer.h"
#include "tty.h"
#include "sys.h"

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
};


static char
tty_getc1 (tty_t *tty)
{
    int ch;

    if (!tty->read (tty->dev, &ch, 1))
        return 0;

    return ch;
}


static int
tty_putc1 (tty_t *tty, int ch)
{
    if (tty->write (tty->dev, &ch, 1) <= 0)
        return -1;

    return ch;
}


int
tty_putc (tty_t *tty, int ch)
{
    /* Convert newline to carriage return/line feed.  */
    if (ch ==  '\n')
        tty_putc1 (tty, '\r');

    return tty_putc1 (tty, ch);
}


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


bool
tty_poll (tty_t *tty)
{
    int ch;

    if (tty->update && !tty->update ())
        return 0;

    ch = tty_getc1 (tty);
    if (!ch)
        return 1;

    /* Echo character.  */
    tty_putc1 (tty, ch);
    if (ch == '\r')
        tty_putc1 (tty, '\n');

    linebuffer_add (tty->linebuffer, ch);

    return 1;
}


int
tty_getc (tty_t *tty)
{
    return linebuffer_getc (tty->linebuffer);
}


char *
tty_gets (tty_t *tty, char *buffer, int size)
{
    return linebuffer_gets (tty->linebuffer, buffer, size);
}


int
tty_printf (tty_t *tty, const char *fmt, ...)
{
    va_list ap;
    int ret;
    int len;
    char buffer[TTY_OUTPUT_BUFFER_SIZE];

    /* Check in case USB detached....  This could happen in the middle
       of a transfer; should make the driver more robust.  */
    if (tty->update && !tty->update ())
        return 0;

    /* FIXME for buffer overrun.  */
    
    va_start (ap, fmt);
    ret = vsnprintf (buffer, sizeof (buffer), fmt, ap);
    va_end (ap);

    return tty_puts (tty, buffer);
}


void
tty_shutdown (tty_t *tty)
{
    if (tty->shutdown)
        tty->shutdown ();
}


tty_t *
tty_init (const tty_cfg_t *cfg, void *dev)
{
    tty_t *tty;

    tty = malloc (sizeof (*tty));
    if (!tty)
        return 0;

    tty->dev = dev;

    tty->read = cfg->read;
    tty->write = cfg->write;
    tty->update = cfg->update;
    tty->shutdown = cfg->shutdown;

    tty->linebuffer = linebuffer_init (TTY_INPUT_BUFFER_SIZE);

    return tty;
}


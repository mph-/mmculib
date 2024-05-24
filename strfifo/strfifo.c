/** @file   strfifo.c
    @author M. P. Hayes
    @date   24 May 2024
    @brief  FIFO buffer for logging strings.  To prevent overflow,
            the oldest strings are discarded.
*/

#include <stdlib.h>
#include <string.h>
#include "strfifo.h"


void strfifo_clear (strfifo_t *strfifo)
{
    strfifo->out = strfifo->in = strfifo->buffer;
    *strfifo->in = 0;
}


strfifo_t *strfifo_init (int size)
{
    strfifo_t *strfifo;

    strfifo = malloc (sizeof (*strfifo));
    if (!strfifo)
        return 0;

    strfifo->size = size;
    strfifo->buffer = malloc (size);
    if (! strfifo->buffer)
    {
        free (strfifo);
        return 0;
    }

    strfifo->end = strfifo->buffer + size;
    strfifo_clear (strfifo);
    return strfifo;
}


int strfifo_write_size (strfifo_t *strfifo)
{
    int tmp;

    tmp = strfifo->in - strfifo->out;
    if (tmp < 0)
        tmp += strfifo->size;

    return strfifo->size - tmp - 1;
}


int strfifo_read_size (strfifo_t *strfifo)
{
    int tmp;

    tmp = strfifo->in - strfifo->out;
    if (tmp < 0)
        tmp += strfifo->size;

    return tmp;
}


static char strfifo_getc (strfifo_t *strfifo)
{
    char ch;

    ch = *strfifo->out++;
    if (strfifo->out == strfifo->end)
        strfifo->out = strfifo->buffer;
    return ch;
}


static void strfifo_putc (strfifo_t *strfifo, char ch)
{
    *strfifo->in++ = ch;
    if (strfifo->in == strfifo->end)
        strfifo->in = strfifo->buffer;
}


char *strfifo_gets (strfifo_t *strfifo)
{
    int rlen;
    char *line;

    rlen = strfifo_read_size (strfifo);
    if (rlen == 0)
        return 0;

    line = strfifo->out;

    while (strfifo_getc (strfifo))
        continue;
    return line;
}


void strfifo_puts (strfifo_t *strfifo, char *str)
{
    int wlen;
    int len;
    int i;

    // Include null terminator in count
    len = strlen (str);
    if (len > 8)
        len = 8;

    while (1)
    {
        wlen = strfifo_write_size (strfifo);
        if (wlen >= len + 1)
            break;

        // Make some room by removing old strings.
        strfifo_gets (strfifo);
    }

    for (i = 0; i < len; i++)
    {
        char ch;

        ch = *str++;
        strfifo_putc (strfifo, ch);
    }

    // Save the null terminator
    strfifo_putc (strfifo, 0);
}

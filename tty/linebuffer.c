#include <stdlib.h>
#include "utility/ring.h"
#include "linebuffer.h"

struct linebuffer_struct
{
    ring_t ring;
};



linebuffer_t *
linebuffer_init (int size)
{
    char *buffer;
    linebuffer_t *linebuffer;
    
    buffer = malloc (size);
    if (!buffer)
        return 0;

    linebuffer = malloc (sizeof (*linebuffer));
    if (!linebuffer)
        return 0;
    
    ring_init (&linebuffer->ring, buffer, size);
    return linebuffer;
}


/** Non-blocking getc.  */
char
linebuffer_getc (linebuffer_t *linebuffer)
{
    return ring_getc (&linebuffer->ring);
}


void
linebuffer_add (linebuffer_t *linebuffer, char ch)
{
    switch (ch)
    {
    case '\b':
        linebuffer_getc (linebuffer);
        break;

    case '\r':
        ring_putc (&linebuffer->ring, '\n');
        break;

    default:
        ring_putc (&linebuffer->ring, ch);
        break;
    }
}


/** Non-blocking gets.  */
char *
linebuffer_gets (linebuffer_t *linebuffer, char *buffer, int size)
{
    int i;

    if (!ring_find (&linebuffer->ring, '\n'))
        return 0;

    for (i = 0; i < size - 1; i++)
    {
        char ch;

        ch = ring_getc (&linebuffer->ring);
        buffer[i] = ch;
        
        if (ch == '\n')
        {
            buffer[i + 1] = 0;
            return buffer;
        }
    }
     
    return buffer;
}

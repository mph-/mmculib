/** @file   linebuffer.h
    @author M. P. Hayes
    @date   12 May 2015
    @brief  A wrapper for a ring buffer with primitive line editing
    support and mapping of carriage returns to newlines.
*/

#include <stdlib.h>
#include "errno.h"
#include "utility/ring.h"
#include "linebuffer.h"

struct linebuffer_struct
{
    ring_t ring;
};


/** Initialise line buffer.
    @param size is the maximum linebuffer size
    @return pointer to linebuffer 
*/
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


/** This is a non-blocking version of fgetc.  
    @param linebuffer a pointer to the linebuffer
    @return next character from line buffer if it contains a newline 
            otherwise -1.
*/
int
linebuffer_getc (linebuffer_t *linebuffer)
{
    if (!ring_find (&linebuffer->ring, '\n'))
    {
        errno = EAGAIN;
        return -1;
    }

    return ring_getc (&linebuffer->ring);
}


/** Add new character to linebuffer.  Backspace will delete the
    previous character in the linebuffer (if present).  Carriage
    returns are converted to newlines.  */
void
linebuffer_add (linebuffer_t *linebuffer, char ch)
{
    switch (ch)
    {
    case '\b':
        /* Discard last character from the linebuffer.  */
        ring_getc (&linebuffer->ring);
        break;

    case '\r':
        /* Replace carriage return with newline.  */
        ring_putc (&linebuffer->ring, '\n');
        break;

    default:
        ring_putc (&linebuffer->ring, ch);
        break;
    }
}


/** This is a non-blocking version of fgets.   If the linebuffer
    contains a newline, then the linebuffer is copied into the user's
    buffer up to and including the newline, provided the user's buffer
    is large enough.
    @param linebuffer a pointer to the linebuffer
    @param buffer is a pointer to a buffer
    @param size is the number of bytes in the buffer
    @return buffer if a line from the linebuffer has been copied otherwise 0.
*/
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

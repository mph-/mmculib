/** @file   logbuffer.c
    @author M. P. Hayes, UCECE
    @date   6 Jan 2020
*/

#include <string.h>
#include <stdlib.h>
#include "logbuffer.h"


logbuffer_t *
logbuffer_init (void *buffer, logbuffer_size_t size)
{
    logbuffer_t *logbuffer;
    
    logbuffer = calloc (1, sizeof (*logbuffer));
    if (logbuffer == 0)
        return 0;

    if (buffer == 0)
    {
        buffer = calloc (1, size);
        if (buffer == 0)
        {
            free (logbuffer);
            return 0;
        }
    }
    
    logbuffer->top = buffer;
    logbuffer->in = buffer;    
    logbuffer->size = size;
    return logbuffer;
}
    

void
logbuffer_append (logbuffer_t *logbuffer, char *str)
{
    int size;
    int left;

    size = strlen (str) + 1;

    left = logbuffer->size - (logbuffer->in - logbuffer->top);

    if (size > left)
        size = left;

    memcpy (logbuffer->in, str, size);
    logbuffer->in += size - 1;

    // Ensure that have null terminator if did not write all the chars.
    *(logbuffer->in) = 0;
}


void
logbuffer_clear (logbuffer_t *logbuffer)
{
    logbuffer->in = logbuffer->top;
    *(logbuffer->in) = 0;    
}    
    
    
void
logbuffer_free (logbuffer_t *logbuffer)
{
    free (logbuffer->top);
    free (logbuffer);    
}

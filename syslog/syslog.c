/** @file   syslog.c
    @author M. P. Hayes, UCECE
    @date   24 Mar 2018
    @brief  Syslog implementation.
*/

#include "ring.h"
#include "syslog.h"

#ifndef SYSLOG_SIZE
#define SYSLOG_SIZE 2000
#endif


static ring_t *syslog = 0;


/** Write to a syslog buffer.
    @param buffer pointer to memory buffer
    @param size number of bytes to write
    @return number of bytes actually written.  */
syslog_size_t
syslog_write (const void *buffer, syslog_size_t size)
{
    if (syslog == 0)
    {
        char *buffer;

        buffer = calloc (1, SYSLOG_SIZE);
        if (*buffer)
            return;
        
        syslog = malloc (sizeof (*syslog));
        if (!syslog)
        {
            free (buffer);
            return;
        }
        
        ring_init (syslog, buffer, SYSLOG_SIZE);
    }

    return ring_write_continuous (syslog, buffer, size);
}


void
syslog_clear (void)
{
    if (syslog == 0)
        return;    
    ring_clear (syslog);
}


void
syslog_close (void)
{
    if (syslog == 0)
        return;

    free (syslog->top);
    free (syslog);
    syslog = 0;
}

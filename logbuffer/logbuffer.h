/** @file   logbuffer.h
    @author M. P. Hayes, UCECE
    @date   24 Mar 2018
    @brief  Syslog implementation.
*/

#ifndef _SYSLOG_H
#define _SYSLOG_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include "config.h"

typedef uint16_t logbuffer_size_t;
    
typedef struct logbuffer
{
    logbuffer_size_t size;
    char *top;
    char *in;
} logbuffer_t;
    

logbuffer_t *
logbuffer_init (void *buffer, logbuffer_size_t size);
    

void
logbuffer_append (logbuffer_t *logbuffer, char *str);


void
logbuffer_clear (logbuffer_t *logbuffer);
    
    
void
logbuffer_free (logbuffer_t *logbuffer);

    
#ifdef __cplusplus
}
#endif    
#endif


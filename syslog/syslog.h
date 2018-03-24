/** @file   syslog.h
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

typedef uint16_t syslog_size_t;


/** Write to a syslog buffer.
    @param buffer pointer to memory buffer
    @param size number of bytes to write
    @return number of bytes actually written.  */
syslog_size_t
syslog_write (const void *buffer, syslog_size_t size);


void
syslog_clear (void);
    
    
void
syslog_close (void);    

    
#ifdef __cplusplus
}
#endif    
#endif


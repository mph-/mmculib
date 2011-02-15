/** @file   tracelog.h
    @author M. P. Hayes, UCECE
    @date   10 May 2010
    @brief 
*/
#ifndef TRACELOG_H
#define TRACELOG_H


void tracelog_printf (const char *fmt, ...);


void tracelog_init (const char *filename);


void tracelog_flush (void);


void tracelog_close (void);

#endif

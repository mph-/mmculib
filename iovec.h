/** @file   iovec.h
    @author Michael Hayes
    @date   10 March 2009
    @brief  I/O vector definitions.
*/
 
#ifndef IOVEC_H
#define IOVEC_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

typedef unsigned int iovec_size_t;

typedef struct iovec
{
    void *data;
    iovec_size_t len;
} iovec_t;


typedef unsigned int iovec_count_t;


#ifdef __cplusplus
}
#endif    
#endif


/** @file   fat_trace.h
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem trace routines for debugging.
*/

#ifndef FAT_TRACE_H
#define FAT_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "trace.h"


#ifndef TRACE_FAT_ERROR
#define TRACE_FAT_ERROR TRACE_PRINTF
#endif


#ifndef TRACE_FAT_INFO
#define TRACE_FAT_INFO  TRACE_PRINTF
#endif


#ifndef TRACE_FAT_DEBUG
#define TRACE_FAT_DEBUG(...)
#endif


#ifdef __cplusplus
}
#endif    
#endif


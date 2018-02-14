/** @file   uint16toa.h
    @author M. P. Hayes, UCECE
    @date   21 Nov 2006
    @brief  16 bit unsigned int to ASCII conversion.
*/

#ifndef UINT16TOA_H
#define UINT16TOA_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

/** Convert 16 bit unsigned integer to ASCII.  */
void
uint16toa (uint16_t num, char *str, bool leading_zeroes);


#ifdef __cplusplus
}
#endif    
#endif


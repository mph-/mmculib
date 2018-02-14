/** @file   uint8toa.h
    @author M. P. Hayes, UCECE
    @date   21 Nov 2006
    @brief  8 bit unsigned int to ASCII conversion.
*/

#ifndef UINT8TOA_H
#define UINT8TOA_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"


/** Convert 8 bit unsigned number to an ASCII string.
    @param num number to convert
    @param str pointer to array of at least 4 chars to hold string
    @param leading_zeroes non-zero to pad with leading zeroes  */
void
uint8toa (uint8_t num, char *str, bool leading_zeroes);


#ifdef __cplusplus
}
#endif    
#endif


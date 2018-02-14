/** @file   uint16tohex.h
    @author M. P. Hayes, UCECE
    @date   21 Nov 2006
    @brief  16 bit unsigned int to ASCII conversion.
*/

#ifndef UINT16TOHEX_H
#define UINT16TOHEX_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

extern void 
uint16tohex (uint16_t number, char *str, unsigned int digits, 
             bool leading_zeros);


#ifdef __cplusplus
}
#endif    
#endif



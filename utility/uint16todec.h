/** @file   uint16todec.h
    @author M. P. Hayes, UCECE
    @date   21 Nov 2006
    @brief  16 bit unsigned int to ASCII conversion.
*/

#ifndef UINT16TODEC_H
#define UINT16TODEC_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

extern void 
uint16todec (uint16_t number, char *str, unsigned int digits, 
             bool leading_zeros);


#ifdef __cplusplus
}
#endif    
#endif



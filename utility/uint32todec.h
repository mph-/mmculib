/** @file   uint32todec.h
    @author M. P. Hayes, UCECE
    @date   21 Nov 2006
    @brief  32 bit unsigned int to ASCII conversion.
*/

#ifndef UINT32TODEC_H
#define UINT32TODEC_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

extern void 
uint32todec (uint32_t number, char *str, unsigned int digits, 
             bool leading_zeros);


#ifdef __cplusplus
}
#endif    
#endif



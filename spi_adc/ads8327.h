/** @file   ads8327.h
    @author M. P. Hayes, UCECE
    @date   09 August 2007
    @brief  ADS8327 SPI ADC
*/

#ifndef ADS8327_H
#define ADS8327_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

extern void
ads8327_init (void);


extern int16_t
ads8327_convert (void);


extern int16_t
ads8327_read (void);

#ifdef __cplusplus
}
#endif    
#endif




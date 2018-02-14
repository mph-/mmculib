/** @file   dsds2450.h
    @author M. P. Hayes, UCECE
    @date   08 June 2002
    @brief 
*/
#ifndef DS2450_H
#define DS2450_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "u1wire.h"

enum {DS2450_CHANNELS_NUM = 4};


extern int8_t
ds2450_adc_conversion_start (u1wire_t dev, uint8_t channel_mask);

extern bool
ds2450_adc_ready_p (u1wire_t dev);

extern int8_t
ds2450_adc_read (u1wire_t dev, uint8_t channel_mask, uint16_t *adc);

/* Return true if the one wire device is a dsds2450 ADC.  */
extern bool
ds2450_device_p (u1wire_obj_t *dev);

extern void
ds2450_debug (u1wire_t dev);

extern u1wire_t
ds2450_init (u1wire_obj_t *dev);

#ifdef __cplusplus
}
#endif    
#endif


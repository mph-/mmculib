/** @file   ads7870.h
    @author M. P. Hayes, UCECE
    @date   09 August 2007
    @brief  ADS7870 SPI ADC
*/

#ifndef ADS7870_H
#define ADS7870_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

extern void
ads7870_init (void);


extern int16_t
ads7870_read (void);


extern int16_t
ads7870_convert (void);


extern void
ads7870_channel_start (uint8_t channel, spi_adc_mode_t mode);


extern int16_t
ads7870_channel_convert (uint8_t channel, spi_adc_mode_t mode);


#ifdef __cplusplus
}
#endif    
#endif




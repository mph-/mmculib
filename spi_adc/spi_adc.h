/** @file   spi_adc.h
    @author M. P. Hayes, UCECE
    @date   09 August 2007
    @brief  SPI ADC
*/

#ifndef SPI_ADC_H
#define SPI_ADC_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"


typedef enum
{
    SPI_ADC_MODE_SINGLE_ENDED,
    SPI_ADC_MODE_DIFFERENTIAL,
    SPI_ADC_MODE_DIFFERENTIAL_INVERTED
} spi_adc_mode_t;



#if defined (SPI_ADC_ADS8327)
#include "ads8327.h"

#define spi_adc_init ads8327_init

#define spi_adc_convert ads8327_convert

#elif defined (SPI_ADC_ADS7870) | defined (SPI_ADC_ADS7871)
#include "ads7870.h"

#define spi_adc_init ads7870_init

#define spi_adc_channel_convert ads7870_channel_convert

#else
#error Undefined SPI_ADC_TYPE
#endif




#ifdef __cplusplus
}
#endif    
#endif




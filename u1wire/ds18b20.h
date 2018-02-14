/** @file   ds18b20.h
    @author M. P. Hayes, UCECE
    @date   08 June 2002
    @brief  Driver for ds18b20 one wire temperature sensor
*/
#ifndef DS18B20_H
#define DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "u1wire.h"

typedef int16_t ds18b20_temp_t;


enum { DS18B20_BITS_PER_DEGREE = 8 };
enum { DS18B20_COUNTS_PER_DEGREE = (1 << 8) };
enum { DS18B20_TEMP_BITS = 12, DS1820_TEMP_BITS = 9 };

/* Round to the nearest degree.  */
#define DS18B20_TEMP_INT(TEMP) \
    (((TEMP) + DS18B20_COUNTS_PER_DEGREE / 2) >> DS18B20_BITS_PER_DEGREE)

#define DS18B20_TEMP_DOUBLE(TEMP) \
        ((TEMP) / (double)DS18B20_COUNTS_PER_DEGREE + 0.5)


/** Start a temperature conversion.
    @param dev pointer to one wire device object
    @return 1 if OK, 0 if no device responding, negative if error
    @note The conversion takes 0.75 s for 12 bit resolution.  */
extern int8_t
ds18b20_temp_conversion_start (u1wire_t dev);


/** Return true when a temperature conversion finished.
    @param dev pointer to one wire device
    @return 1 if conversion finished otherwise 0  */
extern bool
ds18b20_temp_ready_p (u1wire_t dev);


/** Read last converted temperature.
    @param dev pointer to one wire device
    @param ptemp pointer for temperature result
    @return 1 if OK, 0 if no device responding, negative if error
    @note The temperature conversion must be complete.  */
extern int8_t 
ds18b20_temp_read (u1wire_t dev, ds18b20_temp_t *ptemp);


/** Return true if the one wire device is a ds18b20 temperature sensor.
    @param dev pointer to one wire device
    @return 1 if device is a dsb18b20 otherwise 0  */
extern bool
ds18b20_device_p (u1wire_obj_t *dev);


/** Initialise ds18b20 temperature sensor.
    @param dev pointer to one wire device
    @return pointer to one wire device if device is a ds18b20 temperature sensor
    otherwise NULL  */
extern u1wire_t
ds18b20_init (u1wire_obj_t *dev);

#ifdef __cplusplus
}
#endif    
#endif


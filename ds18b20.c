/** @file   ds18b20.c
    @author M. P. Hayes, UCECE
    @date   08 June 2002
    @brief  Driver for ds18b20 one wire temperature sensor
*/
#include <stdio.h>
#include "ds18b20.h"
#include "u1wire.h"
#include "delay.h"
#include "dscrc8.h"

/** Family codes supported by this driver.  */
enum { DS18B20_FAMILY_CODE = 0x28,
       DS1820_FAMILY_CODE = 0x10,
       DS18S20_FAMILY_CODE = 0x10 };

/** Number of bytes including CRC.  */
enum { DS18B20_SCRATCHPAD_BYTES = 9 };

/** One wire bus commands.  */
enum 
{ 
    DS18B20_CONVERT_T = 0x44,
    DS18B20_READ_SCRATCHPAD = 0xbe,
    DS18B20_WRITE_SCRATCHPAD = 0x4e
};


#ifndef DS18B20_DEBUG
#define DS18B20_DEBUG 0
#endif

#ifndef DS18B20_CRC_CHECK
#define DS18B20_CRC_CHECK 0
#endif

/* Store scratchpad data as a global to ease PIC local variable 
   memory allocation.  */
static uint8_t ds18b20_data[DS18B20_SCRATCHPAD_BYTES];


/* There are three steps for reading a temperature as shown by the
   following example:
   
ds18b20_temp_conversion_start (dev);

while (!ds18b20_temp_ready_p (dev))
    continue;

ds18b20_temp_read (dev, &temp);
*/


/** Start a temperature conversion.
    @param dev pointer to one wire device object
    @return 1 if OK, 0 if no device responding, negative if error
    @note The conversion takes 0.75 s for 12 bit resolution.  */
int8_t
ds18b20_temp_conversion_start (u1wire_t dev)
{
    return u1wire_command (dev, DS18B20_CONVERT_T);
}


/** Return true when a temperature conversion finished.
    @param dev pointer to one wire device
    @return 1 if conversion finished otherwise 0  */
bool
ds18b20_temp_ready_p (u1wire_t dev __attribute__ ((unused)))
{
    return u1wire_ready_p ();
}


/** Read last converted temperature.
    @param dev pointer to one wire device
    @param ptemp pointer for temperature result
    @return 1 if OK, 0 if no device responding, negative if error
    @note The temperature conversion must be complete.  */
int8_t
ds18b20_temp_read (u1wire_t dev, ds18b20_temp_t *ptemp)
{
    int8_t ret;
    ds18b20_temp_t temp;

    ret = u1wire_command (dev, DS18B20_READ_SCRATCHPAD);
#if DS18B20_DEBUG
    if (ret != 1)
        printf ("! dsb18b20 read error %d\n", ret);
#endif

    u1wire_read (ds18b20_data, DS18B20_SCRATCHPAD_BYTES);

    temp = (int8_t) ds18b20_data[1] * 256 + ds18b20_data[0];

    /* Left justify the readings so the high byte contains the
       integer temperature value.  Note, the DS1820 and DS18S20 only
       send 9 bits so justify the same as the DS18B20.  */
    if (dev->rom_code.fields.family == DS1820_FAMILY_CODE)
        temp <<= (16 - DS1820_TEMP_BITS);
    else
        temp <<= (16 - DS18B20_TEMP_BITS);

    *ptemp = temp;

#if DS18B20_CRC_CHECK
    {
        crc8_t crc;

        crc = dscrc8 (0, ds18b20_data, sizeof (ds18b20_data) - 1);
        if (crc != ds18b20_data[8])
        {
#if DS18B20_DEBUG
            printf ("! ds18b20 crc error, crc = %u, %u\n", 
                    crc, ds18b20_data[8]);
            printf ("%02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                    ds18b20_data[0], ds18b20_data[1],
                    ds18b20_data[2], ds18b20_data[3], 
                    ds18b20_data[4], ds18b20_data[5],
                    ds18b20_data[6], ds18b20_data[7]);      
#endif
            return U1WIRE_ERR_CRC;
        }
        return 1;
    }
#else

    /* If read all ones, it is likely the sensor is disconnected.
       Alternatively, the room could be on fire!  */
    return ds18b20_data[0] != 255 || ds18b20_data[1] != 255;
#endif
}


/** Return true if the one wire device is a ds18b20 temperature sensor.
    @param dev pointer to one wire device
    @return 1 if device is a dsb18b20 otherwise 0  */
bool
ds18b20_device_p (u1wire_obj_t *dev)
{
    return dev->rom_code.fields.family == DS18B20_FAMILY_CODE
    || dev->rom_code.fields.family == DS1820_FAMILY_CODE;
}


/** Initialise ds18b20 temperature sensor.
    @param dev pointer to one wire device
    @return pointer to one wire device if device is a ds18b20 temperature sensor
    otherwise NULL  */
u1wire_t
ds18b20_init (u1wire_obj_t *dev)
{
    if (ds18b20_device_p (dev))
        return dev;
    return NULL;
}

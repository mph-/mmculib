/** @file   dscrc8.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  8-bit CRC routines for Dallas Semiconductor one wire bus.
*/
#include "dscrc8.h"

/* The CRC uses x^8 + x^5 + x^4 + x^0.  Ignoring x^8
   the mask is 0011001 (0x31).  */


/** Update CRC for a single byte. 
    @param crc CRC value
    @param val value to update CRC with 
    @return new CRC  */
crc8_t
dscrc8_byte (crc8_t crc, uint8_t val)
{
    uint8_t i;

    crc = crc ^ val;
    for (i = 0; i < 8; i++)
    {
        if (crc & 0x01)
            crc = (crc >> 1) ^ 0x8C;
        else
            crc >>= 1;
    }

    return crc;
}


/** Update CRC for a number of bytes. 
    @param crc CRC value
    @param bytes pointer to array of bytes
    @param size number of bytes to process
    @return new CRC  */
crc8_t
dscrc8 (crc8_t crc, uint8_t *bytes, uint8_t size)
{
    uint8_t i;

    for (i = 0; i < size; i++)
        crc = dscrc8_byte (crc, bytes[i]);

    return crc;
}
    



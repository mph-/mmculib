/** @file   dscrc16.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include "dscrc16.h"

/* The CRC uses x^16 + x^15 + x^2 + x^0.  Ignoring x^16
   the mask is 1000000000000101 (0x8005). 

   This CRC is not complemented.  */


static crc16_t 
dscrc16_bit (crc16_t crc, uint8_t in)
{
    uint8_t bit0;

    /* NB, the CRC is stored in reverse order to that specified
       by the polynomial.  */
    bit0 = crc & 1;

    crc >>= 1;    
    if (bit0 ^ in)
        crc = crc ^ (BIT (15 - 0) | BIT (15 - 2) | BIT (15 - 15));

    return crc;
}


crc16_t
dscrc16_byte (crc16_t crc, uint8_t val)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        crc = dscrc16_bit (crc, val & 1);
        val >>= 1;
    }
    return crc;
}


crc16_t
dscrc16 (crc16_t crc, void *bytes, uint8_t size)
{
    uint8_t i;
    uint8_t *data = bytes;

    for (i = 0; i < size; i++)
        crc = dscrc16_byte (crc, data[i]);

    return crc;
}

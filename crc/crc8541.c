/** @file   crc8541.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include "crc8541.h"


static crc8_t 
crc8541_bit (crc8_t crc, uint8_t in)
{
    uint8_t bit7;
    uint8_t foo;

    bit7 = crc >> 7;
    foo = bit7 ^ in;

    crc <<= 1;

    if (foo)
        crc ^= 0x31;

    return crc;
}


crc8_t
crc8541_byte (crc8_t crc, uint8_t val)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        crc = crc8541_bit (crc, val & 1);
        val >>= 1;
    }
    return crc;
}


crc8_t
crc8541 (crc8_t crc, uint8_t *bytes, uint8_t size)
{
    uint8_t i;

    for (i = 0; i < size; i++)
        crc = crc8541_byte (crc, bytes[i]);

    return crc;
}
    



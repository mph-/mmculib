/** @file   uint16toa.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  16 bit unsigned int to ASCII conversion.
*/
#include "config.h"


/** Convert 16 bit unsigned integer to ASCII.  */
void
uint16toa (uint16_t num, char *str, bool leading_zeroes)
{
    uint16_t d;
    uint8_t i;
    uint16_t const powers[] = {10000, 1000, 100, 10, 1, 0};

    for (i = 0; (d = powers[i]); i++)
    {
        uint8_t q;
        
        q = num / d;
        if (leading_zeroes || q || d == 1)
        {
            *str++ = '0' + q;
            num -= q * d;
            leading_zeroes = 1;
        }
    }
    *str = '\0';
}

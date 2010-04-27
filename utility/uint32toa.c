/** @file   uint32toa.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  32 bit unsigned int to ASCII conversion.
*/
#include "config.h"


/** Convert 32 bit unsigned integer to ASCII.  */
void
uint32toa (uint32_t num, char *str, bool leading_zeroes)
{
    uint32_t d;
    uint8_t i;
    uint32_t const powers[] = {10000, 1000, 100, 10, 1, 0};

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

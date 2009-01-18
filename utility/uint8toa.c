/** @file   uint8toa.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include "config.h"

void
uint8toa (uint8_t num, char *str, bool leading_zeroes)
{
    uint8_t d;
    uint8_t i;
    uint8_t const powers[] = {100, 10, 1, 0};

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

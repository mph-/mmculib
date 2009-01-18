/** @file   isqrt16.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include <limits.h>
#include <stdint.h>


uint8_t isqrt (uint16_t val)
{
    uint16_t testdiv;
    uint8_t root;
    uint8_t b;
    uint8_t count;

    root = 0;
    count = sizeof (val) * CHAR_BIT / 2 - 1;
    b = 1 << count;
    
    do {
        testdiv = ((root << 1) + b) << count;
        if (val >= testdiv)
        {
            root += b;
            val -= testdiv;
        }
        b >>= 1;
    } while (count-- != 0);

    return root;
}

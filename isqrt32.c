/** @file   isqrt32.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include <limits.h>
#include <stdint.h>


uint16_t isqrt32 (uint32_t val)
{
    uint32_t testdiv;
    uint16_t root;
    uint16_t b;
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

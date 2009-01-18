/** @file   crc8541_test.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include <stdlib.h>
#include "crc8541.h"


int main (int argc, char **argv)
{
    uint8_t crc;
    uint8_t crc2;
    uint8_t val;
    int i;

    crc = strtol (argv[1], NULL, 0);

    for (i = 2; i < argc; i++)
    {
        val = strtol (argv[i], NULL, 0);
        crc2 = crc8541_byte (crc, val);
        printf ("0x%x 0x%x 0x%x (%d)\n", crc, val, crc2, crc2);
        crc = crc2;
    }

    return 0;
}

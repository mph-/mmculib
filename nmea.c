/** @file   nmea.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include <stdio.h>
#include "config.h"

uint8_t
nmea_checksum (const char *string)
{
    uint8_t checksum = 0;

    if (*string++ != '$')
        return 0;
    
    while (*string)
        checksum ^= *string++;

    return checksum;
}


void nmea_puts (const char *string)
{
    uint8_t checksum;

    checksum = nmea_checksum (string);

    printf ("%s*%02x\r\n", string, checksum);
}

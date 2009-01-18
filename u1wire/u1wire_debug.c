/** @file   u1wire_debug.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include "u1wire.h"
#include <stdio.h>

#ifndef U1WIRE_DEBUG
#define U1WIRE_DEBUG 1
#endif


void
u1wire_debug (u1wire_t dev)
{
#if U1WIRE_DEBUG
    int8_t i;

    printf ("%02x:", dev->rom_code.fields.family);

    for (i = 0; i < ARRAY_SIZE (dev->rom_code.fields.serial); i++)
        printf ("%02x", dev->rom_code.fields.serial[i]);

    printf (":%02x", dev->rom_code.fields.crc);
#endif
}



/** @file   sflash.c
    @author M. P. Hayes, UCECE
    @date   2 July 2007
    @brief 
*/

#include "sflash.h"


/* Return the next state for the associated device.  For example,
   to control the flashing of a LED use:

   led_set (led, sflash_update (sflash));  */
bool
sflash_update (sflash_t sflash)
{
    bool ret;
    sflash_pattern_r pattern;

    pattern = sflash->current;

    if (! pattern)
        pattern = sflash->pattern;

    ret = pattern & 1;
    sflash->current = pattern >> 1;

    return ret;
}

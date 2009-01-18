/** @file   flasher_tweak.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include "flasher.h"

void
flasher_tweak_mod_duty (flasher_pattern_t *pattern, uint8_t mod_duty)
{
    /* Round to nearest integer for duty.  Perhaps should round up?  */
    pattern->mod_duty = (pattern->mod_period * mod_duty + 50) / 100u;
}


void
flasher_tweak_mod_freq (flasher_pattern_t *pattern, uint16_t poll_freq, 
                        uint8_t mod_freq)
{
    uint8_t old_period;

    old_period = pattern->mod_period;
    pattern->mod_period = poll_freq / mod_freq;
    /* Without the cast a signed division is performed.  */
    pattern->mod_duty = (pattern->mod_duty * pattern->mod_period)
        / (uint16_t) old_period;
}

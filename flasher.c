/** @file   flasher.c
    @author M. P. Hayes, UCECE
    @date   13 March 2005
    @brief 
*/

#define FLASHER_TRANSPARENT

#include "flasher.h"


/* These routines are for flashing a LED or beeping a piezo tweeter.
   Perhaps they should be separated into software PWM and flash
   sequencing.  */

int8_t
flasher_pattern_set (flasher_t flasher, flasher_pattern_t *pattern)
{
    flasher->pattern = pattern;
    flasher->mod_count = 0;
    flasher->flasher_count = 0;
    flasher->flashes_count = 0;
    flasher->flasher_prescale = 0;
    return 1;
}


flasher_pattern_t *
flasher_pattern_get (flasher_t flasher)
{
    return flasher->pattern;
}


/* FIXME.  */
int8_t
flasher_phase_set (flasher_t flasher, uint8_t phase)
{
    flasher->mod_count = 0;
    flasher->flasher_count = 0;
    flasher->flashes_count = phase;
    return 1;
}


/* Return the next state for the associated device.  For example,
   to control the flashing of a LED use:

   led_set (led, flasher_update (flasher));  */
bool
flasher_update (flasher_t flasher)
{
    if (!flasher->pattern)
        return 0;

    flasher->mod_count++;
    if (flasher->mod_count >= flasher->pattern->mod_period)
    {
        flasher->mod_count = 0;
        flasher->flasher_prescale++;

        if (flasher->flasher_prescale >= FLASHER_PRESCALE)
        {
            flasher->flasher_prescale = 0;
            flasher->flasher_count++;
            
            if (flasher->flasher_count >= flasher->pattern->flasher_period)
            {
                flasher->flasher_count = 0;
                flasher->flashes_count++;
                
                if (!flasher->pattern->period)
                {
                    /* One shot mode.  */
                    if (flasher->flashes_count >= flasher->pattern->flashes)
                    {
                        /* Disable pattern.  */
                        flasher->pattern = 0;
                        return 1;
                    }
                }
                else if (flasher->flashes_count >= flasher->pattern->period)
                {
                    flasher->flashes_count = 0;
                }
            }
        }
    }

    return flasher->mod_count < flasher->pattern->mod_duty
        && flasher->flasher_count < flasher->pattern->flasher_duty
        && flasher->flashes_count < flasher->pattern->flashes;
}


/* Create a new flasher device.  */
flasher_t
flasher_init (flasher_obj_t *flasher)
{
    flasher_pattern_set (flasher, 0);
    return flasher;
}

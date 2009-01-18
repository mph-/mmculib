/** @file   spwm.c
    @author M. P. Hayes, UCECE
    @date   13 March 2005
    @brief 
*/
#include "config.h"
#include "spwm.h"


/* These routines are for flashing a LED or beeping a piezo tweeter.  */

void
spwm_period_set (spwm_t spwm, uint16_t period)
{
    spwm_obj_t *dev = spwm;

    dev->period = period;
}


void
spwm_duty_set (spwm_t spwm, uint16_t duty)
{
    spwm_obj_t *dev = spwm;

    dev->duty = duty;
}


/* Return the next state for the associated device.  For example,
   to control the flashing of a LED use:

   led_set (led, spwm_update (spwm));  */
bool
spwm_update (spwm_t spwm)
{
    spwm_obj_t *dev = spwm;

    dev->count++;
    if (dev->count >= dev->period)
        dev->count = 0;

    return dev->count < dev->duty;
}


/* Create a new spwm device.  */
spwm_t
spwm_init (spwm_obj_t *dev)
{
    dev->count = 0;
    return dev;
}

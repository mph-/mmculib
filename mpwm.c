/** @file   mpwm.c
    @author M. P. Hayes, UCECE
    @date   1 April 2007
    @brief 
*/
#include "config.h"
#include "mpwm.h"


/* These routines are for flashing a LED or beeping a piezo tweeter.  */

void
mpwm_period_set (mpwm_t mpwm, uint16_t period)
{
    mpwm_obj_t *dev = mpwm;

    dev->period = period;
}


void
mpwm_duty_set (mpwm_t mpwm, uint8_t channel, uint16_t duty)
{
    mpwm_obj_t *dev = mpwm;

    dev->channels[channel].duty = duty;
}


/* Return the next state for the associated device.  For example,
   to control the flashing of a LED use:

   led_set (led, mpwm_update (mpwm));  */
bool
mpwm_update (mpwm_t mpwm)
{
    mpwm_obj_t *dev = mpwm;
    uint8_t i;

    dev->count++;


    if (dev->count >= dev->period)
        dev->count = 0;

    return dev->count < dev->duty;
}


/* Create a new mpwm device.  */
mpwm_t
mpwm_init (mpwm_obj_t *dev, mpwm_channel_t *channels, uint8_t num_channels)
{
    dev->channels = channels;
    dev->num_channels = num_channels;
    dev->period = 0;
    return dev;
}

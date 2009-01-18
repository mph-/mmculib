/** @file   led_flash.c
    @author M. P. Hayes, UCECE
    @date   10 Jan 2006
    @brief 
*/
#include "config.h"
#include "delay.h"
#include "led.h"


/* Flash LED BLINKS times a duty cycle of 50%.  The on period is
   specified by DELAYMS milliseconds.  This hangs until the blinking
   has finished.  */
void
led_flash (led_t led, uint8_t blinks, uint8_t delayms)
{
    int i;

    for (i = 0; i < blinks; i++)
    {
        led_set (led, 1);
        delay_ms (delayms);
        led_set (led, 0);
        delay_ms (delayms);
    }
}

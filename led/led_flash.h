/** @file   led_flash.h
    @author M. P. Hayes, UCECE
    @date   10 Jan 2006
    @brief  LED flashing routine.
*/
#ifndef LED_FLASH_H
#define LED_FLASH_H

#include "config.h"
#include "led.h"

/* Flash LED BLINKS times a duty cycle of 50%.  The on period is
   specified by DELAYMS milliseconds.  This hangs until the blinking
   has finished.  */
extern void
led_flash (led_t led, uint8_t blinks, uint8_t delayms);

#endif

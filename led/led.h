/** @file   led.h
    @author M. P. Hayes, UCECE
    @date   08 June 2002
    @brief  Simple LED driver.
*/
#ifndef LED_H
#define LED_H

#include "config.h"
#include "port.h"


#define LED_CFG PORT_CFG

typedef port_cfg_t led_cfg_t;

typedef const led_cfg_t led_obj_t;

typedef led_obj_t *led_t;


static inline void
led_set (led_t led, uint8_t state)
{
    port_pins_set (led->port, led->bitmask, state);
}


static inline void
led_toggle (led_t led)
{
    port_pins_toggle (led->port, led->bitmask);
}


/* CFG is points to configuration data specified by LED_CFG to
   define the port the LED is connected to.  The returned handle is
   passed to the other led_xxx routines to denote the LED to
   operate on.  */
extern led_t
led_init (const led_cfg_t *cfg);
#endif

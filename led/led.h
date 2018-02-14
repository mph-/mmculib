/** @file   led.h
    @author M. P. Hayes, UCECE
    @date   08 June 2002
    @brief  Simple LED driver.
*/
#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "pio.h"


typedef struct 
{
    pio_t pio;
    /* True for active high, false for active low.  */
    bool active;
} led_cfg_t;

typedef const led_cfg_t led_obj_t;

typedef led_obj_t *led_t;


static inline void
led_set (led_t led, uint8_t state)
{
    pio_output_set (led->pio, led->active ? state : !state);
}


static inline void
led_toggle (led_t led)
{
    pio_output_toggle (led->pio);
}


/* CFG is points to configuration data specified by LED_CFG to
   define the pio the LED is connected to.  The returned handle is
   passed to the other led_xxx routines to denote the LED to
   operate on.  */
extern led_t
led_init (const led_cfg_t *cfg);

#ifdef __cplusplus
}
#endif    
#endif


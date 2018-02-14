/** @file   cleds.h
    @author M. P. Hayes, UCECE
    @date   29 June 2007
    @brief 
*/
#ifndef CLEDS_H
#define CLEDS_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "port.h"
#include "led.h"


typedef struct
{
    const led_cfg_t *leds;
    const led_cfg_t *row_config;
    uint8_t rows_num;
    uint8_t cols_num;
    uint8_t row;
} cleds_private_t;


typedef cleds_private_t cleds_obj_t;
typedef cleds_obj_t *cleds_t;


extern void
cleds_init (cleds_obj_t *cleds, 
            const led_cfg_t *row_config, uint8_t rows_num,
            const led_cfg_t *col_config, uint8_t cols_num);

/* These routines are designed for driving LEDs with separate red,
   green, and blue cathodes.  To activate a LED it is necessary to
   call led_set to drive the common anode high and call led_common_set
   to turn on the cathode driver FET to force the desired cathode low.
   We really only want to activate a single cathode at a time
   otherwise the common anode current might be too high for the
   microcontroller.  Rather than activating multiple cathodes at once,
   for independent LED colour we can time multiplex the cathodes and
   activate the desired anodes at the correct time.
*/

/* Select the desired row.  If row is zero disable all rows. 
   This returns the old row enabled.  */
extern uint8_t
cleds_common_set (cleds_t cleds, uint8_t row);


static inline void
cleds_set (cleds_t cleds, uint8_t id, uint8_t val)
{
    led_set ((led_t)(cleds->leds + id), val);
}


static inline uint8_t
cleds_cols_num_get (cleds_t cleds)
{
    return cleds->cols_num;
}


static inline uint8_t
cleds_rows_num_get (cleds_t cleds)
{
    return cleds->rows_num;
}


static inline uint8_t
cleds_active_row_get (cleds_t cleds)
{
    return cleds->row;
}


static inline uint8_t
cleds_common_cycle (cleds_t cleds)
{
    uint8_t newrow = cleds->row + 1;

    if (newrow > cleds->rows_num)
        newrow = 1;

    cleds_common_set (cleds, newrow);
    return newrow;
}

#ifdef __cplusplus
}
#endif    
#endif


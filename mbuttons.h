/** @file   mbuttons.h
    @author M. P. Hayes, UCECE
    @date   29 June 2007
    @brief 
*/

#ifndef MBUTTONS_H
#define MBUTTONS_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "button.h"

/* This supports multiplexed buttons; either as a matrix or a vector.
   Each row of the matrix is driven low in turn and then each button
   column is polled for that row.

   The buttons are enumerated as button = row * cols_num + col.  */


typedef struct
{
    button_t buttons;
    const button_cfg_t const *row_config;
    uint8_t rows_num;
    uint8_t cols_num;
} mbuttons_private_t;


typedef mbuttons_private_t mbuttons_obj_t;
typedef mbuttons_obj_t *mbuttons_t;

extern void
mbuttons_init (mbuttons_obj_t *mbuttons,
               button_obj_t *mbuttons_info, 
               const button_cfg_t *row_config, uint8_t rows_num,
               const button_cfg_t *col_config, uint8_t cols_num,
               uint8_t poll_count);


extern void
mbuttons_poll (mbuttons_t mbuttons);


extern bool
mbuttons_any_state_p (mbuttons_t mbuttons, button_state_t state);


static inline bool
mbuttons_any_pushed_p (mbuttons_t mbuttons)
{
    return mbuttons_any_state_p (mbuttons, BUTTON_STATE_PUSHED);
}


extern bool
mbuttons_any_down_p (mbuttons_t mbuttons);


extern void
mbuttons_wakeup_init (mbuttons_t mbuttons);


static inline bool
mbuttons_pushed_p (mbuttons_t mbuttons, uint8_t id)
{
    return button_pushed_p (mbuttons->buttons + id);
}


static inline bool
mbuttons_released_p (mbuttons_t mbuttons, uint8_t id)
{
    return button_released_p (mbuttons->buttons + id);
}


static inline bool
mbuttons_held_p (mbuttons_t mbuttons, uint8_t id, uint8_t hold_time)
{
    return button_held_p (mbuttons->buttons + id, hold_time);
}


static inline bool
mbuttons_hold_released_p (mbuttons_t mbuttons, uint8_t id, uint8_t hold_time)
{
    return button_hold_released_p (mbuttons->buttons + id, hold_time);
}


static inline bool
mbuttons_wakeup_p (mbuttons_t mbuttons)
{
    uint8_t i;
    button_t buttons = mbuttons->buttons;

    for (i = 0; i < mbuttons->cols_num; i++)
        if (button_pressed_p (buttons++))
            return 1;

    return 0;
}


#ifdef __cplusplus
}
#endif    
#endif




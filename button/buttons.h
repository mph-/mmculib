/** @file   buttons.h
     @author M. P. Hayes, UCECE
     @date   17 Nov 2006
    @brief   Multiple button polling and debouncing.
*/

#ifndef BUTTONS_H
#define BUTTONS_H

#include "button.h"


/** Private multiple button structure. 

    @note These elements should be considered private so do not access them.
 */
typedef struct
{
    button_t buttons;
    uint8_t num;
} buttons_private_t;


typedef buttons_private_t buttons_obj_t;
typedef buttons_obj_t *buttons_t;


/** Initialise multiple buttons.  */
extern void
buttons_init (buttons_obj_t *buttons, button_obj_t *buttons_info, 
              const button_cfg_t *config, uint8_t buttons_num,
              uint8_t poll_count);

/** Poll multiple buttons.  */
extern void
buttons_poll (buttons_t buttons);

/** Return true if any buttons pushed.  */
extern bool
buttons_any_pushed_p (buttons_t buttons);

/** Return true if selected button pushed.  */
static inline bool
buttons_pushed_p (buttons_t buttons, uint8_t id)
{
    return button_pushed_p (buttons->buttons + id);
}

/** Return true if selected button released.  */
static inline bool
buttons_released_p (buttons_t buttons, uint8_t id)
{
    return button_released_p (buttons->buttons + id);
}

/** Return true if selected button held for hold_count.  */
static inline bool
buttons_held_p (buttons_t buttons, uint8_t id, uint8_t hold_count)
{
    return button_held_p (buttons->buttons + id, hold_count);
}

/** Return true if selected button held for hold_count is released.  */
static inline bool
buttons_hold_released_p (buttons_t buttons, uint8_t id, uint8_t hold_count)
{
    return button_hold_released_p (buttons->buttons + id, hold_count);
}

#endif

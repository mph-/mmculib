/** @file   buttons.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  Multiple pushbutton driver.
*/
#include "buttons.h"

void
buttons_init (buttons_obj_t *buttons, button_obj_t *button_objs, 
              const button_cfg_t *config, uint8_t buttons_num,
              uint8_t poll_count)
{
    int8_t i;
    
    buttons->buttons = button_objs;
    buttons->num = buttons_num;

    /* Initialise buttons.  */
    for (i = 0; i < buttons_num; i++)
        button_init (button_objs + i, config + i);
    
    button_poll_count_set (poll_count);
}


void
buttons_poll (buttons_t buttons)
{
    int8_t i;
    
    /* Poll buttons.  */
    for (i = 0; i < buttons->num; i++)
        button_poll (buttons->buttons + i);
}


bool
buttons_any_pushed_p (buttons_t buttons)
{
    uint8_t i;

    for (i = 0; i < buttons->num; i++)
        if (button_pushed_p (buttons->buttons + i))
            return 1;

    return 0;
}

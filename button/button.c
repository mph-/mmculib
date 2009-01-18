/** @file   button.c
    @author M.P. Hayes
    @date   16 Feb 2003
    @brief Button polling and debouncing.
    @note This debounces pushbuttons and switches using a polled
     wait-and-see method implemented with a state machine.  It assumes
     that a pushed button gives a logic low on the input port.
*/

#include "button.h"

static uint8_t button_poll_count;


/** Set the number of polls required for the debounce period.
    @param poll_count number of update periods  */
void button_poll_count_set (uint8_t poll_count)
{
    button_poll_count = poll_count;
}


/** Create a new button object.
    @param obj  pointer to button structure
    @param cfg  pointer to button configuration
    @return     pointer to button object  */
button_t
button_init (button_obj_t *obj, const button_cfg_t *cfg)
{
    button_t button = obj;

    button->cfg = cfg;
    button->state = BUTTON_STATE_UP;
    button->count = 0;
    button->hold_count = 0;

    /* Configure port for input and enable internal pullup resistor.  */
    port_pins_config_pullup (cfg->port, cfg->bitmask);

    return button;
}


/** Debounce specified button and return its debounced state.
    @param  button pointer to button object
    @param  pressed true if button current pressed
    @return button state  */
static button_state_t
button_debounce (button_t button, bool pressed)
{
    button_state_t state;

    state = button->state;
    switch (state)
    {
    case BUTTON_STATE_UP:
        if (pressed)
        {
            button->count++;
            if (button->count > button_poll_count)
                state = BUTTON_STATE_PUSHED;            
        }
        else
            button->count = 0;
        break;

    case BUTTON_STATE_PUSHED:
        button->hold_count = 0;
        state = BUTTON_STATE_DOWN;
        break;

    case BUTTON_STATE_DOWN:
        if (pressed)
        {
            if (button->hold_count != 255)
                button->hold_count++;
            button->count = 0;
        }
        else
        {
            button->count++;
            if (button->count > button_poll_count)
                state = BUTTON_STATE_RELEASED;          
        }
        break;

    case BUTTON_STATE_RELEASED:
        state = BUTTON_STATE_UP;
        break;
    }
    button->state = state;
    return state;
}


/** Poll the specified button and return its debounced state.
    @param  button pointer to button object
    @return button state  */
button_state_t
button_poll (button_t button)
{
    return button_debounce (button, button_pressed_p (button));
}

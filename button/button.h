/**  @file   button.h
     @author M. P. Hayes, UCECE
     @date   15 Feb 2003
     @brief  Button polling and debouncing.
*/


#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif
    


#include "config.h"
#include "pio.h"



/** Debounce period in milliseconds.  */
enum {BUTTON_DEBOUNCE_MS = 50};

#define BUTTON_DEBOUNCE_RATE (1000 / BUTTON_DEBOUNCE_MS)

#define BUTTON_POLL_COUNT(POLL_RATE) ((POLL_RATE) / BUTTON_DEBOUNCE_RATE)

#define BUTTON_CFG(PIO) {PIO}


/* Button configuration structure.  */
typedef struct
{
    pio_t pio;
} button_cfg_t;


/** Button states.  */
typedef enum {BUTTON_STATE_UP, BUTTON_STATE_DOWN,
              BUTTON_STATE_PUSHED, BUTTON_STATE_RELEASED} button_state_t;


typedef struct button_struct
{
    button_state_t state;
    pio_t pio;
    uint8_t count;
    uint8_t hold_count;
} button_dev_t;

typedef button_dev_t *button_t;


/** Set the number of polls required for the debounce period.
    @param poll_count number of update periods  */
extern void 
button_poll_count_set (uint8_t poll_count);


/** Create a new button object.
    @param cfg  pointer to button configuration
    @return     pointer to button object  */
extern button_t
button_init (const button_cfg_t *cfg);


/** Poll the specified button and return its debounced state. 
    @param  button pointer to button object
    @return button state  */
extern button_state_t
button_poll (button_t button);


/** Return button state.
    @param  button pointer to button object
    @return button state  */
static inline button_state_t

button_state_get (button_t button)
{
    return button->state;
}


/** Return duration button held (in update periods).
    @param  button pointer to button object
    @return duration button held  */
static inline uint8_t 
button_hold_count_get (button_t button)
{
    return button->hold_count;
}


/** Return true if button pushed. 
    @param  button pointer to button object
    @return 1 if button pushed otherwise 0  */
static inline bool
button_pushed_p (button_t button)
{
    return button_state_get (button) == BUTTON_STATE_PUSHED;
}


/** Return true if button released.
    @param  button pointer to button object
    @return 1 if button released otherwise 0  */
static inline bool
button_released_p (button_t button)
{
    return button_state_get (button) == BUTTON_STATE_RELEASED;
}


/** Return true if button down (pressed).
    @param  button pointer to button object
    @return 1 if button down otherwise 0  */
static inline bool
button_down_p (button_t button)
{
    return button_state_get (button) == BUTTON_STATE_DOWN;
}


/** Return true if button up.
    @param  button pointer to button object
    @return 1 if button down otherwise 0  */
static inline bool
button_up_p (button_t button)
{
    return button_state_get (button) == BUTTON_STATE_UP;
}


/** Return true if button held for hold_count.
    @param  button pointer to button object
    @param  hold_count hold duration (in update periods) 
    @return 1 if button held for specified count otherwise 0  */
static inline bool
button_held_p (button_t button, uint8_t hold_count)
{
    return button_down_p (button)
        && button_hold_count_get (button) > hold_count;
}


/** Return true if button held for hold_count has been released.
    @param  button pointer to button object
    @param  hold_count hold duration (in update periods) 
    @return 1 if button held for specified count is now released otherwise 0  */
static inline bool
button_hold_released_p (button_t button, uint8_t hold_count)
{
    return button_released_p (button)
        && button_hold_count_get (button) > hold_count;
}


/** Return true if button possibly pressed. 
    @param  button pointer to button object
    @return 1 if button currently pressed otherwise 0
    @note This is not debounced.  For a debounced version use
          \c button_pushed_p.  */
static inline bool
button_pressed_p (button_t button)
{
    /* When a button is pushed it pulls the pio line low.  */
    return !pio_input_get (button->pio);
}

void
button_poll_all (void);

#ifdef __cplusplus
}
#endif    
#endif


/** @file   tweeter.c
    @author M. P. Hayes, UCECE
    @date   20 April 2007
    @brief  Generate PWM for a piezo tweeter.
*/

#define TWEETER_TRANSPARENT 1

#include "tweeter.h"


#define TWEETER_HOLDOFF_TIME 50e-3

enum {TWEETER_SCALE_SIZE = 12};


void 
tweeter_note_set (tweeter_t tweeter, 
                  tweeter_period_t period,
                  tweeter_period_t duty)
{
    tweeter->note_period = period;
    tweeter->note_duty = duty;

    /* To distinguish between two quarter notes and a half note of the
       same pitch we have a short hold off period at the start of each
       note.  */
    tweeter->note_holdoff = tweeter->poll_rate
        / (uint16_t)(1 / TWEETER_HOLDOFF_TIME);
}


/* The note and velocity are specified as per the MIDI standard except
   a note value of 0 indicates a rest (note off).  The velocity has a
   maximum value of 127 and gives an indication of the note
   volume.  */
void
tweeter_note_play (tweeter_t tweeter, tweeter_note_t note, uint8_t velocity)
{
    tweeter_period_t period;
    tweeter_period_t duty;
    uint8_t index;
    uint8_t octave;

    if (note == 0)
    {
        /* Play a rest.  */
        tweeter_note_set (tweeter, 0, 0);
        return;
    }

    /* See if we can play this note.  */
    if (note < TWEETER_NOTE_MIN)
        return;

    note -= TWEETER_NOTE_MIN;
    octave = note / TWEETER_SCALE_SIZE;
    index = note - octave * TWEETER_SCALE_SIZE;

    period = tweeter->scale_table[index];
    
    while (octave-- > 0)
        period >>= 1;

    duty = (period * velocity) >> 8;

    tweeter_note_set (tweeter, period, duty);

#if 0
    printf ("note = %d, octave = %d, period = %d\n",
            note, index / TWEETER_SCALE_SIZE, period);
#endif

}


int8_t
tweeter_update (tweeter_t tweeter)
{
    if (tweeter->note_holdoff)
    {
        tweeter->note_holdoff--;
        return 0;
    }

    if (++tweeter->note_clock >= tweeter->note_period)
        tweeter->note_clock = 0;
    
    return tweeter->note_clock < tweeter->note_duty;
}


tweeter_t
tweeter_init (tweeter_obj_t *tweeter, 
              uint16_t poll_rate,
              tweeter_scale_t *scale_table)
{
    tweeter->poll_rate = poll_rate;
    tweeter->scale_table = scale_table;
    tweeter->note_period = 0;
    tweeter->note_duty = 0;

    return tweeter;
}

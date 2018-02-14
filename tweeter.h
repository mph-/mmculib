/** @file   tweeter.h
    @author M. P. Hayes, UCECE
    @date   20 April 2007
    @brief  Generate PWM for a piezo tweeter.
*/
#ifndef TWEETER_H
#define TWEETER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "font.h"
#include "ticker.h"

typedef uint8_t tweeter_note_t;
typedef uint8_t tweeter_duration_t;
typedef uint8_t tweeter_period_t;
typedef uint8_t tweeter_velocity_t;
typedef uint8_t tweeter_scale_t;

/* Could calculate scale divisors at run time.  2^(1/2) is approx
   1.0594631.  A reasonable rational approximation is 267/252 =
   1.0595238.  Let's save memory and provide a macro to compute the
   divisors.  */

#define TWEETER_DIVISOR(POLL_RATE, FREQ) (POLL_RATE / FREQ + 0.5)


enum {TWEETER_NOTE_MIN = 40};
/* Define divisors for chromatic scale E2 -> D#3.  For better accuracy
   this should be defined for the lowest frequency scale, however,
   this may need 16 bits per note.  */
#define TWEETER_SCALE_TABLE(POLL_RATE) \
   {TWEETER_DIVISOR (POLL_RATE, 82.41), \
    TWEETER_DIVISOR (POLL_RATE, 87.31), \
    TWEETER_DIVISOR (POLL_RATE, 92.50), \
    TWEETER_DIVISOR (POLL_RATE, 98.00), \
    TWEETER_DIVISOR (POLL_RATE, 103.83), \
    TWEETER_DIVISOR (POLL_RATE, 110.0), \
    TWEETER_DIVISOR (POLL_RATE, 116.54), \
    TWEETER_DIVISOR (POLL_RATE, 123.47), \
    TWEETER_DIVISOR (POLL_RATE, 130.81), \
    TWEETER_DIVISOR (POLL_RATE, 138.59), \
    TWEETER_DIVISOR (POLL_RATE, 146.83), \
    TWEETER_DIVISOR (POLL_RATE, 155.56)}


typedef struct
{
    uint8_t note_clock;
    uint8_t note_period;
    uint8_t note_duty;
    uint16_t note_holdoff;
    uint16_t poll_rate;
    tweeter_scale_t *scale_table;
} tweeter_private_t;


typedef tweeter_private_t tweeter_obj_t;
typedef tweeter_obj_t *tweeter_t;

/* The scale table is usually defined with:

 static tweeter_scale_t scale_table[] = TWEETER_SCALE_TABLE (LOOP_POLL_RATE);
*/

extern int8_t 
tweeter_update (tweeter_t tweeter);

/* The note and velocity are specified as per the MIDI standard except
   a note value of 0 indicates a rest (note off).  The velocity has a
   maximum value of 127 and gives an indication of the note
   volume.  */
extern void
tweeter_note_play (tweeter_t tweeter, tweeter_note_t note, uint8_t velocity);

extern tweeter_t
tweeter_init (tweeter_obj_t *dev, 
              uint16_t poll_rate,
              tweeter_scale_t *scale_table);


#ifdef __cplusplus
}
#endif    
#endif


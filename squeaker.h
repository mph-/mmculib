/** @file   squeaker.h
    @author M. P. Hayes, UCECE
    @date   14 April 2007
    @brief  Play simple tunes with PWM.
*/
#ifndef SQUEAKER_H
#define SQUEAKER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "font.h"
#include "ticker.h"

typedef uint8_t squeaker_speed_t;
typedef uint8_t squeaker_scale_t;
typedef uint8_t squeaker_note_t;
typedef uint8_t squeaker_duration_t;
typedef uint8_t squeaker_period_t;
typedef uint8_t squeaker_volume_t;


enum {SQUEAKER_OCTAVE_DEFAULT = 4};
enum {SQUEAKER_SPEED_DEFAULT = 200};


/* Could calculate scale divisors at run time.  2^(1/2) is approx
   1.0594631.  A reasonable rational approximation is 267/252 =
   1.0595238.  Let's save memory and provide a macro to compute the
   divisors.  */

#define SQUEAKER_DIVISOR(POLL_RATE, FREQ) (POLL_RATE / FREQ + 0.5)


#if 0
enum {SQUEAKER_NOTE_MIN = 60};
/* Define divisors for chromatic scale C4 -> C5.  For better accuracy
   this should be defined for the lowest frequency scale, however,
   this may need 16 bits per note.  */
#define SQUEAKER_SCALE_TABLE(POLL_RATE) \
   {SQUEAKER_DIVISOR (POLL_RATE, 261.6256), \
    SQUEAKER_DIVISOR (POLL_RATE, 277.1826), \
    SQUEAKER_DIVISOR (POLL_RATE, 293.6648), \
    SQUEAKER_DIVISOR (POLL_RATE, 311.1270), \
    SQUEAKER_DIVISOR (POLL_RATE, 329.6276), \
    SQUEAKER_DIVISOR (POLL_RATE, 349.2282), \
    SQUEAKER_DIVISOR (POLL_RATE, 369.9944), \
    SQUEAKER_DIVISOR (POLL_RATE, 391.9954), \
    SQUEAKER_DIVISOR (POLL_RATE, 415.3047), \
    SQUEAKER_DIVISOR (POLL_RATE, 440.0000), \
    SQUEAKER_DIVISOR (POLL_RATE, 466.1638), \
    SQUEAKER_DIVISOR (POLL_RATE, 493.8833)}
#else
enum {SQUEAKER_NOTE_MIN = 40};
/* Define divisors for chromatic scale E2 -> D#3.  For better accuracy
   this should be defined for the lowest frequency scale, however,
   this may need 16 bits per note.  */
#define SQUEAKER_SCALE_TABLE(POLL_RATE) \
   {SQUEAKER_DIVISOR (POLL_RATE, 82.41), \
    SQUEAKER_DIVISOR (POLL_RATE, 87.31), \
    SQUEAKER_DIVISOR (POLL_RATE, 92.50), \
    SQUEAKER_DIVISOR (POLL_RATE, 98.00), \
    SQUEAKER_DIVISOR (POLL_RATE, 103.83), \
    SQUEAKER_DIVISOR (POLL_RATE, 110.0), \
    SQUEAKER_DIVISOR (POLL_RATE, 116.54), \
    SQUEAKER_DIVISOR (POLL_RATE, 123.47), \
    SQUEAKER_DIVISOR (POLL_RATE, 130.81), \
    SQUEAKER_DIVISOR (POLL_RATE, 138.59), \
    SQUEAKER_DIVISOR (POLL_RATE, 146.83), \
    SQUEAKER_DIVISOR (POLL_RATE, 155.56)}
#endif

typedef struct
{
    uint8_t note_clock;
    uint8_t note_period;
    uint8_t note_duty;
    uint8_t note_holdoff;
    /* Pointer to start of string.  */
    const char *start;
    /* Pointer to current position in string.  */    
    const char *cur;
    uint8_t holdoff;
    uint16_t poll_rate;
    const char *loop_start;
    int8_t loop_count;
    uint8_t prescaler;
    uint8_t note_fraction;
    squeaker_speed_t speed;
    squeaker_volume_t volume;
    ticker8_t ticker;
    squeaker_scale_t *scale_table;
    uint8_t octave;
} squeaker_private_t;


typedef squeaker_private_t squeaker_obj_t;
typedef squeaker_obj_t *squeaker_t;


/* The scale table is usually defined with:

 static squeaker_scale_t scale_table[] = SQUEAKER_SCALE_TABLE (LOOP_POLL_RATE);
*/


extern squeaker_t
squeaker_init (squeaker_obj_t *dev, 
               uint16_t poll_rate,
               squeaker_scale_t *scale_table);

extern void 
squeaker_play (squeaker_t squeaker, const char *str);

extern int8_t 
squeaker_update (squeaker_t squeaker);

/* Set (base) speed in beats per minute (BPM).  */
extern void 
squeaker_speed_set (squeaker_t squeaker, squeaker_speed_t speed);

/* Set volume as percentage of maximum.  */
extern void 
squeaker_volume_set (squeaker_t squeaker, squeaker_volume_t volume);


#ifdef __cplusplus
}
#endif    
#endif


/** @file   sflash.h
    @author M. P. Hayes, UCECE
    @date   2 July 2007
    @brief 
*/
#ifndef SFLASH_H
#define SFLASH_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"


typedef uint16_t sflash_pattern_t;


/* This structure is defined here so the compiler can allocate enough
   memory for it.  However, its fields should be treated as
   private.  */
typedef struct
{
    sflash_pattern_t pattern;
    sflash_pattern_t current;
} sflash_obj_t;


typedef sflash_obj_t *sflash_t;


/* These routines are for sequencing the flashing of a LED or beeping
   a piezo tweeter.  Each pattern is 16 bits in length.  The current
   LSB of a pattern determines whether the LED should be lit.  The
   pattern is then shifted to the right.  When it is zero, it is
   reloaded.  Thus the patterns are repeating and the length of the
   sequence can be fewer than 16 bits.

   For example,
     0bxxxxxxxxxxxxxxx1 will keep a LED on
     0b0000000000000010 will toggle the LED every period
     0b0000000000001100 will toggle the LED every second period
     0b0000000011110000 will toggle the LED every fourth period
     0b1111111100000000 will toggle the LED every eighth period
     0x1010101010000000 will flash the LED 5 times out of 16 periods
 */


/* Set the flash pattern and the initial sequence.  If you don't care
   about the phasing use 0 or pattern for initial.  */
static inline void
sflash_pattern_set (sflash_t sflash, sflash_pattern_t pattern,
                    sflash_pattern_t initial)
{
    sflash->pattern = pattern;
    sflash->current = initial;
}


static inline sflash_pattern_t
sflash_pattern_get (sflash_t sflash)
{
    return sflash->pattern;
}

extern bool
sflash_update (sflash_t);


#ifdef __cplusplus
}
#endif    
#endif


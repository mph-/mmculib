/** @file   flasher.h
    @author M. P. Hayes, UCECE
    @date   13 March 2005
    @brief 
*/
#ifndef FLASHER_H
#define FLASHER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

/* This parameter is for internal use only.  It's purpose is to 
   reduce the chances of flasher_period and flasher_duty overflow.
   It could possibly be made an additional parameter for flasher_pattern_t
   and calculated by FLASHER_PATTERN.  */
#define FLASHER_PRESCALE 8

/* POLL_RATE (Hz)
   MOD_FREQ (Hz)
   MOD_DUTY (percent)
   FLASHER_PERIOD (s)      - period between flashes
   FLASHER_DUTY (percent)  - proportion of flash period that is lit
   FLASHES (integer)       - how many flashes per flash pattern
   PERIOD (s)              - how often the flash pattern repeats
*/
   
#define FLASHER_PATTERN(POLL_RATE, MOD_FREQ, MOD_DUTY, FLASHER_PERIOD,  \
                          FLASHER_DUTY, FLASHES, PERIOD)                \
    (POLL_RATE) / (double)(MOD_FREQ) + 0.5,                             \
    (POLL_RATE) * (double)(MOD_DUTY) / (MOD_FREQ) / 100.0 + 0.5,        \
    (MOD_FREQ) * (FLASHER_PERIOD) / (double)FLASHER_PRESCALE + 0.5,     \
    (MOD_FREQ) * (FLASHER_PERIOD) * (FLASHER_DUTY) / 100.0 / FLASHER_PRESCALE + 0.5, \
    (FLASHES),                                                          \
    (PERIOD) / (double)(FLASHER_PERIOD) + 0.5


typedef struct
{
    /* This is the modulation period.  It determines the frequency
       of a tone or flicker rate of a LED.  */
    uint8_t mod_period;
    /* This is the modulation duty.  It determines the luminance of a LED.  */
    uint8_t mod_duty;
    /* This is the period between the start of two flashes in a sequence.  */
    uint8_t flasher_period;
    /* This is the flash period.  */
    uint8_t flasher_duty;
    /* This is the number of flashes in the sequence.  */
    uint8_t flashes;
    /* This is the number of flasher periods before the sequence repeats.  */
    uint8_t period;
} flasher_pattern_t;


#define FLASHER_ACTIVE_P(FLASHER) \
    (((flasher_obj_t *)(FLASHER))->pattern != 0)


#define FLASHER_PATTERN_FLASHES_SET(PATTERN, FLASHES) \
    (PATTERN)->flashes = (FLASHES)

/* This structure is defined here so the compiler can allocate enough
   memory for it.  However, its fields should be treated as
   private.  */
typedef struct
{
    flasher_pattern_t *pattern;
    uint8_t mod_count;
    uint8_t flasher_count;
    uint8_t flashes_count;
    uint8_t flasher_prescale;
} flasher_private_t;


#ifdef FLASHER_TRANSPARENT
typedef flasher_private_t flasher_obj_t;
#else
typedef struct {char dummy[sizeof (flasher_private_t)];} flasher_obj_t;
#endif
typedef flasher_obj_t *flasher_t;


extern int8_t
flasher_pattern_set (flasher_t flasher, flasher_pattern_t *pattern);

extern flasher_pattern_t *
flasher_pattern_get (flasher_t flasher);

extern int8_t
flasher_phase_set (flasher_t flasher, uint8_t phase);

extern bool
flasher_update (flasher_t);


/* INFO is a pointer into RAM that stores the state of the FLASH.
   CFG is a pointer into ROM to define the port the FLASH is connected to.
   The returned handle is passed to the other flasher_xxx routines to denote
   the FLASH to operate on.  */
extern flasher_t
flasher_init (flasher_obj_t *info);

#ifdef __cplusplus
}
#endif    
#endif


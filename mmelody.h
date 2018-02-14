/** @file   mmelody.h
    @author M. P. Hayes, UCECE
    @date   20 April 2007
    @brief  Play simple melodies.
*/
#ifndef MMELODY_H
#define MMELODY_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "font.h"
#include "ticker.h"

typedef uint8_t mmelody_speed_t;
typedef uint8_t mmelody_scale_t;
typedef uint8_t mmelody_note_t;
typedef uint8_t mmelody_volume_t;


enum {MMELODY_OCTAVE_DEFAULT = 4};
enum {MMELODY_SPEED_DEFAULT = 200};


typedef struct
{
    ticker_t ticker;
    /* Pointer to current position in string.  */    
    const char *cur;
    /* Pointer to start of string.  */
    const char *start;
    const char *loop_start;
    int8_t loop_count;
    uint8_t note_fraction;
    mmelody_speed_t speed;
    mmelody_volume_t volume;
    uint8_t octave;
    void (* play_callback) (void *data, uint8_t note, uint8_t volume);
    void *play_callback_data;
    uint16_t poll_rate;
} mmelody_private_t;

typedef mmelody_private_t mmelody_obj_t;
typedef mmelody_obj_t *mmelody_t;


typedef void (* mmelody_callback_t) (void *data, uint8_t note, uint8_t volume);

extern mmelody_t
mmelody_init (mmelody_obj_t *dev, 
              uint16_t poll_rate,
              mmelody_callback_t play_callback,
              void *play_callback_data);

extern void 
mmelody_play (mmelody_t mmelody, const char *str);

extern void
mmelody_update (mmelody_t mmelody);

/* Set (base) speed in beats per minute (BPM).  */
extern void 
mmelody_speed_set (mmelody_t mmelody, mmelody_speed_t speed);

/* Set volume as percentage of maximum.  */
extern void 
mmelody_volume_set (mmelody_t mmelody, mmelody_volume_t volume);


#ifdef __cplusplus
}
#endif    
#endif


/** @file   chaser.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#ifndef CHASER_H
#define CHASER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "flasher.h"
#include "font.h"


typedef char chaser_font_index_t;
typedef chaser_font_index_t *chaser_sequence_t;

typedef enum {CHASER_MODE_NORMAL, 
              CHASER_MODE_CYCLE, 
              CHASER_MODE_INVERT,
              CHASER_MODE_CYCLE_INVERT,
              CHASER_MODE_NUM} chaser_mode_t;

typedef struct
{
    uint8_t flasher_num;
    flasher_t *flashers;
    chaser_sequence_t seq;
    font_t *font;
    uint8_t step;
    int8_t dir;
    chaser_mode_t mode;
    flasher_pattern_t *on_pattern;
    flasher_pattern_t *off_pattern;
} chaser_private_t;


typedef chaser_private_t chaser_obj_t;
typedef chaser_obj_t *chaser_t;


extern chaser_t
chaser_init (chaser_obj_t *dev, flasher_t *flashers,
             uint8_t flasher_num);

extern void 
chaser_sequence_set (chaser_t chaser, chaser_sequence_t seq);


extern void 
chaser_mode_set (chaser_t chaser, chaser_mode_t mode);


/* Returns non-zero at end of sequence.  */
extern int8_t
chaser_update (chaser_t chaser);


static inline void
chaser_patterns_set (chaser_t chaser,
                     flasher_pattern_t *on_pattern,
                     flasher_pattern_t *off_pattern)
{
    chaser_private_t *dev = chaser;

    dev->on_pattern = on_pattern;
    dev->off_pattern = off_pattern;
}


static inline chaser_sequence_t
chaser_sequence_get (chaser_t chaser)
{
    chaser_private_t *dev = chaser;

    return dev->seq;
}


static inline void
chaser_font_set (chaser_t chaser, font_t *font)
{
    chaser_private_t *dev = chaser;

    dev->font = font;
}


#ifdef __cplusplus
}
#endif    
#endif


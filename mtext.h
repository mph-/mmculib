/** @file   mtext.h
    @author M. P. Hayes, UCECE
    @date   8 April 2007
    @brief  Moving text
*/
#ifndef MTEXT_H
#define MTEXT_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "font.h"
#include "scroller.h"
#include "ticker.h"


enum {MTEXT_SPEED_SCALER = 16};


/* This is currently unused.  */
typedef enum {MTEXT_MODE_REPLACE, MTEXT_MODE_SCROLL} mtext_mode_t;


typedef struct
{
    const char *cur;
    const char *start;
    font_t *font;
    uint8_t pixels;
    uint8_t *image;
    uint8_t *screen;
    uint16_t poll_rate;
    mtext_mode_t mode;
    scroller_obj_t scroller;
    uint8_t speed;
    ticker_t ticker;
} mtext_obj_t;

typedef mtext_obj_t *mtext_t;


/* image and screen must point to static arrays.  image must be the
   size of the current font and is used to store the current character
   to be displayed.  screen holds the actual screen status.  */
extern mtext_t
mtext_init (mtext_obj_t *dev, 
            uint16_t poll_rate,
            font_t *font, uint8_t *image, 
            uint8_t *screen, uint8_t rows, uint8_t cols);

/* Returns non-zero if screen possibly changed.  */
extern int8_t 
mtext_update (mtext_t mtext);

extern void
mtext_scroller_dir_set (mtext_t mtext, scroller_dir_t dir);

extern scroller_dir_t
mtext_scroller_dir_get (mtext_t mtext);

extern void
mtext_speed_set (mtext_t mtext, uint8_t speed);


/* Set the string to display.  NB, the string pointed to by STR is not
   copied and thus must be static.  Control sequences can be embedded
   in the string of the form %<dir>D %<speed>S where <dir> is an
   integer in the range 0--4 and <speed> is an integer.  A % can be
   obtained with %%.  By default, strings are repeatedly displayed.
   This can be avoided by appending the pause sequence %.  For example,
   "HELLO%."
*/
static inline void
mtext_set (mtext_t mtext, const char *str)
{
    mtext->start = mtext->cur = str;
}


static inline const char *
mtext_get (mtext_t mtext)
{
    return mtext->start;
}


static inline void
mtext_mode_set (mtext_t mtext, mtext_mode_t mode)
{
    mtext->mode = mode;
}


static inline mtext_mode_t
mtext_mode_get (mtext_t mtext)
{
    return mtext->mode;
}


#ifdef __cplusplus
}
#endif    
#endif


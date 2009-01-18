/** @file   chaser.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include <limits.h>
#include "chaser.h"
#include "flasher.h"
#include "font.h"

/* Each chaser can control a number of flashers where each flasher
   usually controls a LED.  The font provides a mapping between the
   next character in the chaser sequence and the flashers (and thus
   the LEDs) to activate.  Currently, only two flash patterns are
   supported; one pattern for the "on" LEDs and one for the "off"
   LEDs.  */


chaser_t
chaser_init (chaser_obj_t *dev, flasher_t *flashers,
             uint8_t flasher_num)
{
    dev->flasher_num = flasher_num;
    dev->flashers = flashers;
    dev->step = 0;
    dev->dir = 1;
    dev->mode = CHASER_MODE_NORMAL;
    return dev;
}


void
chaser_sequence_set (chaser_t chaser, chaser_sequence_t seq)
{
    chaser_obj_t *dev = chaser;

    dev->seq = seq;
    dev->step = 0;
    dev->dir = 1;
}


void
chaser_mode_set (chaser_t chaser, chaser_mode_t mode)
{
    chaser_obj_t *dev = chaser;

    dev->mode = mode;
    dev->step = 0;
    dev->dir = 1;
}


static void
chaser_pixel_set (void *data, font_t *font, uint8_t col, uint8_t row, bool val)
{
    chaser_obj_t *dev = data;
    bool invert;

    invert = dev->mode == CHASER_MODE_INVERT 
        || dev->mode == CHASER_MODE_CYCLE_INVERT;

    flasher_pattern_set (dev->flashers[row * font->width + col], val ^ invert 
                         ? dev->on_pattern : dev->off_pattern);
}


int8_t
chaser_update (chaser_t chaser)
{
    chaser_obj_t *dev = chaser;
    bool cycle;

    if (!dev->seq)
        return 0;

    cycle = dev->mode == CHASER_MODE_CYCLE
        || dev->mode == CHASER_MODE_CYCLE_INVERT;

    font_display (dev->seq[dev->step], dev->font,
                  chaser_pixel_set, dev);

    /* Update chaser state.  */
    if (dev->dir > 0)
    {
        /* Moving forward.  */
        dev->step++;

        if (!dev->seq[dev->step])
        {
            if (! cycle)
            {
                /* Reached end of sequence.  */
                dev->step = 0;
                return 1;
            }
            dev->dir = -1;
            dev->step -= 2;
        }
    }
    else
    {
        /* Moving backward.  */
        if (!dev->step)
        {
            dev->dir = 1;
            dev->step = 1;
            return 1;
        }
        else
        {
            dev->step--;
        }
    }
    return 0;
}

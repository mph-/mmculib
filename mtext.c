/** @file   mtext.c
    @author M. P. Hayes, UCECE
    @date   8 April 2007
    @brief  Moving text
*/

#include "mtext.h"
#include "font.h"
#include <limits.h>


static void
mtext_pixel_set (void *data, font_t *font, uint8_t col, uint8_t row, bool val)
{
    uint8_t *image = data;

    image[row * font->width + col] = val;
}


static void
mtext_display (mtext_t mtext, char ch)
{
    font_display (ch, mtext->font, mtext_pixel_set, mtext->image);
}


static const char *
mtext_scan (mtext_t mtext, const char *str)
{
    uint8_t num;

    /* Loop until end of string or until have processed a normal
       char.  */
    while (1)
    {
        /* If at end of sequence, repeat from start.  */
        if (! *str)
            str = mtext->start;

        if (*str != '%')
        {
            mtext_display (mtext, *str++);
            break;
        }

        /* Pause.  */
        if (*++str == '.')
        {
            /* Ensure we don't skip past %.  */
            str--;
            break;
        }

        if (*str == '%')
        {
            mtext_display (mtext, '%');
            str++;
            break;
        }

        num = 0;
        while (*str >= '0' && *str <= '9')
            num = num * 10 + *str++ - '0';
        
        switch (*str++)
        {
            /* %<dir>D  .*/
        case 'D':
            mtext_scroller_dir_set (mtext, num);
            break;
            
            /* %<mode>M  .*/
        case 'M':
            mtext_mode_set (mtext, num);
            break;
            
            /* %<speed>S  .*/
        case 'S':
            /* The speed is relative to the base speed and scaled by
               MTEXT_SPEED_SCALER, so with MTEXT_SPEED_SCALER = 16, 32 is
               twice as fast while 8 is twice as slow.  */
            mtext_speed_set (mtext, num);
            break;
            
            /* We could have a user callback to handle other escape
               sequences such as beeps.  */
            
        default:
            break;
        }
    }

    return str;
}


mtext_t
mtext_init (mtext_obj_t *mtext, uint16_t poll_rate,
            font_t *font, uint8_t *image, 
            uint8_t *screen, uint8_t rows, uint8_t cols)
{
    mtext->cur = 0;
    mtext->font = font;
    mtext->pixels = font->rows * font->cols;
    mtext->image = image;
    mtext->screen = screen;
    mtext->poll_rate = poll_rate;
    /* I might deprecate modes.  */
    mtext->mode = MTEXT_MODE_SCROLL;

    scroller_init (&mtext->scroller, rows, cols, SCROLLER_OFF);

    /* Update display at 2 Hz.  */
    mtext_speed_set (mtext, 2 * MTEXT_SPEED_SCALER);

    return mtext;
}


void
mtext_scroller_dir_set (mtext_t mtext, scroller_dir_t dir)
{
    scroller_dir_set (&mtext->scroller, dir);

    mtext_speed_set (mtext, mtext->speed);
}


scroller_dir_t
mtext_scroller_dir_get (mtext_t mtext)
{
    return scroller_dir_get (&mtext->scroller);
}


/* Returns non-zero if screen possibly changed.  */
int8_t mtext_update (mtext_t mtext)
{
    if (! TICKER_UPDATE (&mtext->ticker))
        return 0;

    if (scroller_update (&mtext->scroller, mtext->image, mtext->screen))
    {
        if (mtext->cur)
            mtext->cur = mtext_scan (mtext, mtext->cur);
        scroller_start (&mtext->scroller, mtext->image, mtext->screen);
    }       
    
    return 1;
}


void
mtext_speed_set (mtext_t mtext, uint8_t speed)
{
    uint16_t speed16;

    mtext->speed = speed;
    
    speed16 = speed * scroller_speed_scale_get (&mtext->scroller);

    TICKER_INIT (&mtext->ticker, mtext->poll_rate * MTEXT_SPEED_SCALER 
                 / speed16);
}

/** @file   stext.c
    @author M. P. Hayes, UCECE
    @date   2 April 2007
    @brief  Sequenced text
*/

#include "stext.h"
#include <limits.h>


static const char *
stext_display (void *data, const char *str)
{
    stext_obj_t *stext = data;

    font_display (str[0], stext->font, stext->callback, stext->callback_data);
    return ++str;
}


stext_t
stext_init (stext_obj_t *stext, 
            font_t *font,
            void (*callback) (void *data, uint8_t pixel, bool val),
            void *callback_data)
{
    stext->font = font;
    stext->callback = callback;
    stext->callback_data = callback_data;
    stext->seq = seq_init (&stext->seq_info, stext_display, stext);

    return stext;
}

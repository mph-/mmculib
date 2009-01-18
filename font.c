/** @file   font.c
    @author M. P. Hayes, UCECE
    @date   1 March 2007
    @brief  Font support.
*/

#include "font.h"

/** Call callback function for every pixel in font.
    @param ch character to display
    @param font point to font structure 
    @param display pointer to callback function
    @param data pointer to pass to callback function
    @return 1 if ch in font otherwise 0  */
bool
font_display (char ch, font_t *font,
              font_callback_t display,
              void *data)
{
    int8_t index;
    font_elt_t *font_elt;
    uint8_t i;
    uint8_t j;
    uint8_t k;
    uint8_t bytes_elt;
    uint8_t font_byte;
    
    /* Point to font entry.  */
    index = ch - font->offset;

    if (index < 0 || index >= font->size)
        return 0;

    bytes_elt = (font->width * font->height + 8 - 1) >> 3;

    font_elt = &font->data[index * bytes_elt];

    /* Iterate over all pixels.  */
    k = 0;
    font_byte = *font_elt++;
    
    for (j = 0; j < font->height; j++)
    {
        for (i = 0; i < font->width; i++)
        {
            display (data, font, i, j, (font_byte & 1) != 0);
            font_byte >>= 1;
            k++;
            if (k >= 8)
            {
                k = 0;
                font_byte = *font_elt++;
            }
        }
    }
    return 1;
}

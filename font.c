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
    font_data_t *char_data;
    uint8_t i;
    uint8_t j;
    uint8_t k;
    uint8_t bytes_per_char;
    uint8_t char_byte;
    uint8_t pixels_per_byte;
    
    /* Point to font entry.  */
    index = ch - font->offset;

    if (index < 0 || index >= font->size)
        return 0;

    bytes_per_char = (font->width * font->height + 8 - 1) >> 3;

    pixels_per_byte = 8;
    /* If bit 0 of font->flags not set need to rotate font.  */

    char_data = &font->data[index * bytes_per_char];

    /* Iterate over all pixels.  */
    k = 0;
    char_byte = *char_data++;
    
    for (j = 0; j < font->height; j++)
    {
        for (i = 0; i < font->width; i++)
        {
            display (data, font, i, j, (char_byte & 1) != 0);
            char_byte >>= 1;
            k++;
            if (k >= pixels_per_byte)
            {
                k = 0;
                char_byte = *char_data++;
            }
        }
    }
    return 1;
}

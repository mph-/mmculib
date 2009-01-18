/** @file   font.h
    @author M. P. Hayes, UCECE
    @date   1 March 2007
    @brief  Font support.
*/

#ifndef FONT_H
#define FONT_H

#include "config.h"


typedef const uint8_t font_elt_t;

/** Font structure.  */
typedef const struct
{
    /* Flags for future options.  */
    uint8_t flags;
    /* Width of font char.  */
    uint8_t width;
    /* Height of font char.  */
    uint8_t height;
    /* Index of first entry in font.  */
    uint8_t offset;
    /* Number of font entries in table.  */
    uint8_t size;
    /* Font data entries.  */
    font_elt_t data[];
} font_t;


/* We could have multiple font entries concatenated in memory with
   different offsets/sizes to allow fonts with holes to save
   memory.  */

typedef void (* font_callback_t) (void *data, font_t *font,
                                  uint8_t col, uint8_t row, bool val);

/** Call callback function for every pixel in font.
    @param ch character to display
    @param font point to font structure 
    @param display pointer to callback function
    @param data pointer to pass to callback function
    @return 1 if @ref ch in font otherwise 0  */
extern bool
font_display (char ch, font_t *font,
              font_callback_t display,
              void *data);
#endif

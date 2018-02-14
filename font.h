/** @file   font.h
    @author M. P. Hayes, UCECE
    @date   1 March 2007
    @brief  Font support.
*/

#ifndef FONT_H
#define FONT_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "fontdef.h"

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

#ifdef __cplusplus
}
#endif    
#endif


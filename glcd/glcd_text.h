/** @file   glcd_text.h
    @author M. P. Hayes, UCECE
    @date   16 Febraury 2008
    @brief  Simple text display routines for GLCD.
*/
#ifndef GLCD_TEXT_H
#define GLCD_TEXT_H

#include "glcd.h"
#include "font.h"

extern void
glcd_text (glcd_t glcd, font_t *font, uint8_t col, uint8_t row, const char *str);


#endif

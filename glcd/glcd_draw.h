/** @file   glcd_draw.h
    @author M. P. Hayes, UCECE
    @date   16 Febraury 2008
    @brief  Simple line drawing routines for GLCD.
*/
#ifndef GLCD_DRAW_H
#define GLCD_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "glcd.h"

void
glcd_draw_line (glcd_t glcd, uint8_t x_start, uint8_t y_start, 
                uint8_t x_end, uint8_t y_end, bool val);


#ifdef __cplusplus
}
#endif    
#endif


/** @file   textview.h
    @author M. P. Hayes, UCECE
    @date   16 Febraury 2008
    @brief  Simple text viewing routines for GLCD.
*/
#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "font.h"


typedef void (* textview_pixel_set_t) (void *data, uint8_t col, uint8_t row,
                                       uint8_t val);

typedef void (* textview_update_t) (void *data);



typedef struct
{
    void *data;
    char *screen;
    font_t *font;
    uint8_t rows;
    uint8_t cols;
    textview_pixel_set_t pixel_set;
    textview_update_t update;
    uint8_t row;
    uint8_t col;
    uint8_t flags;
} textview_private_t;

typedef textview_private_t textview_obj_t;
typedef textview_obj_t *textview_t;


textview_t
textview_init (textview_obj_t *obj, char *screen, uint8_t cols, uint8_t rows, 
               font_t *font,
               textview_pixel_set_t pixel_set,
               textview_update_t update, void *data);


void
textview_goto (textview_t this, uint8_t col, uint8_t row);

void
textview_clear (textview_t this);

void
textview_redraw (textview_t this);

void
textview_font_set (textview_t this, font_t *font);

void
textview_putc (textview_t this, char ch);

void
textview_puts (textview_t this, const char *string);

void
textview_wrap_set (textview_t this, uint8_t enable);

void
textview_update (textview_t this);


#ifdef __cplusplus
}
#endif    
#endif


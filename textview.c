/** @file   textview.c
    @author M. P. Hayes, UCECE
    @date   16 February 2008
    @brief  Simple text viewing routines for GLCD.  This module implements
            a scrolling character display.
*/

#include "textview.h"
#include "glcd_text.h"

#include <stdlib.h>


enum {TEXTVIEW_FLAG_WRAP = BIT (0)};


void
textview_goto (textview_t this, uint8_t col, uint8_t row)
{
    this->row = row;
    this->col = col;
}


void
textview_clear (textview_t this)
{
    uint8_t row;
    uint8_t col;

    for (row = 0; row < this->rows; row++)
        for (col = 0; col < this->cols; col++)
            this->screen[row * this->cols + col] = ' ';

    this->row = 0;
    this->col = 0;

    glcd_clear (this->data);
}


textview_t
textview_init (textview_obj_t *obj,
               char *screen,
               uint8_t cols,
               uint8_t rows,
               font_t *font,
               textview_pixel_set_t pixel_set, 
               textview_update_t update, 
               void *data)
{
    textview_t this;
    
    this = obj;

    this->data = data;
    this->rows = rows;
    this->cols = cols;
    this->screen = screen;
    this->pixel_set = pixel_set;
    this->update = update;
    this->font = font;

    textview_clear (this);
    return this;
}


/* This implements a jumpy scroll; perhaps should add a smooth scroll.  */
void
textview_scroll (textview_t this)
{
    uint8_t row;
    uint8_t col;

    /* Scroll screen characters.  */
    for (row = 1; row < this->rows; row++)
        for (col = 0; col < this->cols; col++)
            this->screen[(row - 1) * this->cols + col]
                = this->screen[row * this->cols + col];

    /* Blank bottom line.  */
    for (col = 0; col < this->cols; col++)
        this->screen[(this->rows - 1) * this->cols + col] = ' ';

    this->row--;

    textview_redraw (this);
}


void
textview_font_set (textview_t this, font_t *font)
{
    this->font = font;
    textview_redraw (this);
}


void
textview_redraw (textview_t this)
{
    uint8_t row;
    uint8_t col;

    for (row = 0; row < this->rows; row++)
        for (col = 0; col < this->cols; col++)
        {
            char buffer[2];

            buffer[0] = this->screen[row * this->cols + col];
            buffer[1] = '\0';
            glcd_text (this->data, this->font, col * (this->font->width + 1),
                       row * (this->font->height + 1), buffer);
        }
}


static void
textview_putc_1 (textview_t this, char ch)
{
    char buffer[2];

    if (this->row >= this->rows)
        textview_scroll (this);

    switch (ch)
    {
    case '\r':
        this->col = 0;
        break;
        
    case '\n':
        this->col = 0;
        this->row++;
        /* Defer the scrolling.  */
        break;
        
    default:
        if (this->col >= this->cols)
        {
            if (this->flags & TEXTVIEW_FLAG_WRAP)
                textview_putc_1 (this, '\n');
            else
                break;
        }

        /* Force character to uppercase if no font support for lowercase.  */
        if (ch > this->font->offset + this->font->size)
            ch -= 'a' - 'A';
        buffer[0] = ch;
        buffer[1] = '\0';
        this->screen[this->row * this->cols + this->col] = ch;
        glcd_text (this->data, this->font, this->col * (this->font->width + 1),
                   this->row * (this->font->height + 1), buffer);
        this->col++;
        break;
    }
}


void
textview_putc (textview_t this, char ch)
{
    textview_putc_1 (this, ch);
    glcd_update (this->data);
}


void
textview_puts (textview_t this, const char *string)
{
    while (*string)
    {
        textview_putc_1 (this, *string++);
    }
}


void
textview_wrap_set (textview_t this, uint8_t enable)
{
    if (enable)
        this->flags |= TEXTVIEW_FLAG_WRAP;
    else
        this->flags &= ~TEXTVIEW_FLAG_WRAP;
}

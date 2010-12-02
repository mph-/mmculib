/** @file   mdraw.c
    @author M. P. Hayes, UCECE
    @date   16 February 2008
    @brief  Simple drawing routines for GLCD.
*/

#include "mdraw.h"
#include <stdlib.h>

mdraw_t
mdraw_init (mdraw_obj_t *obj,
            mdraw_pixel_set_t pixel_set, 
            mdraw_update_t update, 
            void *data)
{
    mdraw_t this;
    
    this = obj;

    this->data = data;
    this->pixel_set = pixel_set;
    this->update = update;
    this->x = 0;
    this->y = 0;

    return this;
}


void
mdraw_move (mdraw_t this, uint8_t x, uint8_t y)
{
    this->x = x;
    this->y = y;
}


void
mdraw_line (mdraw_t this, uint8_t x_end, uint8_t y_end, uint8_t val)
{
    int dx;
    int dy;
    int x_inc;
    int y_inc;
    int x;
    int y;

    /* Draw a line using Bresenham's algorithm.  */

    dx = x_end - this->x;
    dy = y_end - this->y;

    x_inc = dx >= 0 ? 1 : -1;
    y_inc = dy >= 0 ? 1 : -1;

    dx = abs (dx);
    dy = abs (dy);

    if (abs (dx) >= abs (dy))
    {
        int c;

        c = dx / 2;
        y = this->y;
        for (x = this->x; x != x_end; x += x_inc)
        {
            c += dy;
            if (c >= dx)
            {
                c -= dx;
                y += y_inc;
            }
            this->pixel_set (this->data, x, y, val);
        }
    }
    else
    {
        int c;

        c = dy / 2;
        x = this->x;
        for (y = this->y; y != y_end; y += y_inc)
        {
            c += dx;
            if (c >= dy)
            {
                c -= dy;
                x += x_inc;
            }
            this->pixel_set (this->data, x, y, val);
        }
    }
    this->x = x_end;
    this->y = y_end;
}


void
mdraw_update (mdraw_t this)
{
    this->update (this->data);
}


void 
mdraw_plot (mdraw_t this, uint8_t *data, uint8_t size, uint8_t offset, uint8_t val)
{
    uint8_t x;

    mdraw_move (this, offset, data[0]);
    for (x = 1; x < size; x++)
        mdraw_line (this, x, data[x], val);
}

#include "glcd_draw.h"
#include <stdlib.h>

void
glcd_draw_line (glcd_t glcd, uint8_t x_start, uint8_t y_start, 
                uint8_t x_end, uint8_t y_end, bool val)
{
    int dx;
    int dy;
    int x_inc;
    int y_inc;
    int x;
    int y;

    /* Draw a line using Bresenham's algorithm.  */

    dx = x_end - x_start;
    dy = y_end - y_start;

    x_inc = dx >= 0 ? 1 : -1;
    y_inc = dy >= 0 ? 1 : -1;

    dx = abs (dx);
    dy = abs (dy);

    if (abs (dx) >= abs (dy))
    {
        int c = 0;

        y = y_start;
        for (x = x_start; x != x_end; x += x_inc)
        {
            c += dy;
            if (c >= dx)
            {
                c -= dx;
                y += y_inc;
            }
            glcd_pixel_set (glcd, x, y, val);
        }
    }
    else
    {
        int c = 0;

        x = x_start;
        for (y = y_start; y != y_end; y += y_inc)
        {
            c += dx;
            if (c >= dy)
            {
                c -= dy;
                x += x_inc;
            }
            glcd_pixel_set (glcd, x, y, val);
        }
    }
}




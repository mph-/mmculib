/** @file   scroller.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#include "scroller.h"
#include <string.h>


scroller_t
scroller_init (scroller_t scroller, int rows, int cols, scroller_dir_t dir)
{
    scroller->rows = rows;
    scroller->cols = cols;
    scroller->dir = dir;
    scroller->index = 0;
    scroller->running = 0;
    return scroller;
}


int8_t
scroller_update (scroller_t scroller, uint8_t *image, uint8_t *screen)
{
    uint8_t rows = scroller->rows;
    uint8_t cols = scroller->cols;
    uint8_t index = scroller->index;
    uint8_t i;
    uint8_t j;
    int8_t ret = 1;

    if (!scroller->running)
        return 1;

    switch (scroller->dir)
    {
    case SCROLLER_OFF:
        return 1;

    case SCROLLER_LEFT:
        /* Scroll display across cols.  */ 
        for (i = 0; i < cols - 1; i++)
        {
            for (j = 0; j < rows; j++)
                screen[j * cols + i]
                    = screen[j * cols + i + 1];
        }
        
        if (index < cols)
        {
            /* Shift in new image.  */
            for (j = 0; j < rows; j++)
                screen[j * cols + cols - 1] = image[j * cols + index];
            ret = 0;
        }
        else
        {
            for (j = 0; j < rows; j++)
                screen[j * cols + cols - 1] = 0;
        }
        break;
        

    case SCROLLER_RIGHT:
        /* Scroll display across cols.  */ 
        for (i = 0; i < cols - 1; i++)
        {
            for (j = 0; j < rows; j++)
                screen[j * cols + (cols - 1 - i)]
                    = screen[j * cols + (cols - 2 - i)];
        }
    
        /* Shift in new image.  */
        if (index < cols)
        {
            for (j = 0; j < rows; j++)
                screen[j * cols] = image[j * cols + (cols - 1 - index)];
            ret = 0;
        }
        else
        {
            for (j = 0; j < rows; j++)
                screen[j * cols] = 0;
        }
        break;

    case SCROLLER_UP:
        /* Scroll display across rows.  */ 
        for (j = 0; j < rows - 1; j++)
        {
            for (i = 0; i < cols; i++)
                screen[j * cols + i]
                    = screen[(j + 1) * cols + i];
        }

        if (index < rows)
        {
            /* Shift in new image.  */
            for (i = 0; i < cols; i++)
                screen[(rows - 1) * cols + i] = image[index * cols + i];
            ret = 0;
        }
        else
        {
            for (i = 0; i < cols; i++)
                screen[(rows - 1) * cols + i] = 0;
        }
        break;

    case SCROLLER_DOWN:
        /* Scroll display across rows.  */ 
        for (j = 0; j < rows - 1; j++)
        {
            for (i = 0; i < cols; i++)
                screen[(rows - 1 - j) * cols + i]
                    = screen[(rows - 2 - j) * cols + i];
        }
        
        if (index < rows)
        {
            /* Shift in new image.  */
            for (i = 0; i < cols; i++)
                screen[i] = image[(rows - 1 - index) * cols + i];
            ret = 0;
        }
        else
        {
            for (i = 0; i < cols; i++)
                screen[i] = 0;
        }
        break;
    }

    if (! ret)
        scroller->index = index + 1;

    return ret;
}


void
scroller_start (scroller_t scroller, uint8_t *image, uint8_t *screen)
{
    scroller->index = 0;
    scroller->running = 1;

    if (scroller->dir == SCROLLER_OFF)
        memcpy (screen, image, scroller->rows * scroller->cols);
}


uint8_t
scroller_speed_scale_get (scroller_t scroller)
{
    switch (scroller->dir)
    {
    case SCROLLER_OFF:
        return 1;
    case SCROLLER_LEFT:
    case SCROLLER_RIGHT:
        return scroller->cols + 1;
    case SCROLLER_UP:
    case SCROLLER_DOWN:
        return scroller->rows + 1;

    default:
        return 0;
    }
}

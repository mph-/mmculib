#include "glcd.h"
#include "font.h"

typedef struct
{
    uint8_t row;
    uint8_t col;
    glcd_t glcd;
} glcd_text_t;


static void
display_pixel (glcd_text_t *info, font_t *font __UNUSED__,
               uint8_t col, uint8_t row, bool val)
{
    glcd_pixel_set (info->glcd, info->col + col, info->row + row, val);
}


/* This has no effect until glcd_update called.  */
void
glcd_text (glcd_t glcd, font_t *font, uint8_t col, uint8_t row, const char *str)
{
    glcd_text_t info;
    
    info.glcd = glcd;
    info.row = row;
    info.col = col;

    while (*str)
    {
        unsigned int i;

        font_display (*str++, font, (font_callback_t) display_pixel, &info);

        /* Clear pixels along right edge.  */
        for (i = 0; i < font->height; i++)
            display_pixel (&info, font, font->width, i, 0);

        /* Clear pixels along bottom edge.  */
        for (i = 0; i <= font->width; i++)
            display_pixel (&info, font, i, font->height, 0);

        info.col += font->width + 1;
    }
}

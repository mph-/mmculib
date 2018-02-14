/** @file   mdraw.h
    @author M. P. Hayes, UCECE
    @date   16 Febraury 2008
    @brief  Simple line drawing routines for GLCD.
*/
#ifndef MDRAW_H
#define MDRAW_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"


typedef void (* mdraw_pixel_set_t) (void *data, uint8_t col, uint8_t row,
                                    uint8_t val);

typedef void (* mdraw_update_t) (void *data);



typedef struct
{
    void *data;
    mdraw_pixel_set_t pixel_set;
    mdraw_update_t update;
    uint8_t x;
    uint8_t y;
} mdraw_private_t;

typedef mdraw_private_t mdraw_obj_t;
typedef mdraw_obj_t *mdraw_t;


extern mdraw_t
mdraw_init (mdraw_obj_t *obj, mdraw_pixel_set_t pixel_set,
            mdraw_update_t update, void *data);


extern void
mdraw_move (mdraw_t this, uint8_t x, uint8_t y);


extern void
mdraw_line (mdraw_t this, uint8_t x_end, uint8_t y_end, uint8_t val);


void
mdraw_update (mdraw_t this);


extern void 
mdraw_plot (mdraw_t this, uint8_t *data, uint8_t size, uint8_t offset, uint8_t val);


#ifdef __cplusplus
}
#endif    
#endif


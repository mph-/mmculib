/** @file   scroller.h
    @author M. P. Hayes, UCECE
    @date   7 April 2007
    @brief  Image scroller
*/
#ifndef SCROLLER_H
#define SCROLLER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"


typedef enum {SCROLLER_OFF, SCROLLER_LEFT, SCROLLER_RIGHT, 
              SCROLLER_DOWN, SCROLLER_UP} scroller_dir_t;

typedef struct
{
    uint8_t rows;
    uint8_t cols;
    uint8_t index;
    scroller_dir_t dir;
    bool running;
} scroller_obj_t;


typedef scroller_obj_t *scroller_t;

extern scroller_t
scroller_init (scroller_t scroller, int rows, int cols, scroller_dir_t dir);

extern int8_t
scroller_update (scroller_t scroller, uint8_t *image, uint8_t *screen);

extern void
scroller_start (scroller_t scroller, uint8_t *image, uint8_t *screen);

extern uint8_t
scroller_speed_scale_get (scroller_t scroller);

static inline void
scroller_stop (scroller_t scroller)
{
    scroller->running = 0;
}


static inline void
scroller_dir_set (scroller_t scroller, scroller_dir_t dir)
{
    scroller->dir = dir;
}


static inline scroller_dir_t
scroller_dir_get (scroller_t scroller)
{
    return scroller->dir;
}


#ifdef __cplusplus
}
#endif    
#endif


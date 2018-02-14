/** @file   mcleds.h
    @author M. P. Hayes, UCECE
    @date   3 July 2007
    @brief 
*/
#ifndef MCLEDS_H
#define MCLEDS_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "cleds.h"
#include "colourmap.h"
#include "ticker.h"


/* These routines are designed for multiplexing multiple colour LEDs
   with separate red, green, and blue cathodes.  The strategy used
   here is to continuously sequence the R, G, and B cathodes while the
   anodes are driven for a portion of each R, G, B period to control
   the proportion of red, blue, and green comprising each colour
   determined by a colourmap.

   For example, let's assume that the cathodes are asserted for 10
   clocks and we want a colour comprised of 6/10 red, 5/10 green, and
   8/10 blue we would generate the following timing sequence, where X
   denotes when a signal is asserted:

   R: XXXXXXXXXX                    XXXXXXXXXX
   G:           XXXXXXXXXX                    XXXXXXXXXX
   B:                     XXXXXXXXXX
   A: XXXXXX    XXXXX     XXXXXXXX  XXXXXX    XXXXX  
*/


typedef struct
{
    uint8_t duty;
} mcleds_state_t;


typedef struct
{
    cleds_obj_t cleds;
    colourmap_t *colourmap;
    uint8_t colourmap_size;
    ticker8_t primary_ticker;
    mcleds_state_t *state;
} mcleds_private_t;


typedef mcleds_private_t mcleds_obj_t;
typedef mcleds_obj_t *mcleds_t;


/* State is an array of cols_num length for storing state for each led.  */
extern mcleds_t
mcleds_init (mcleds_obj_t *mcleds, 
             const led_cfg_t *row_config, uint8_t rows_num,
             const led_cfg_t *col_config, uint8_t cols_num,
             colourmap_t *colourmap, uint8_t colourmap_size,
             mcleds_state_t *state, uint8_t update_rate);


/* Screen is an array of colourmap indexes.  Non-zero is returned
   when the colour drive is cycled.  */
extern bool
mcleds_update (mcleds_t mcleds, uint8_t *screen);


extern void
mcleds_off (mcleds_t mcleds);


extern void
mcleds_enable (mcleds_t mcleds, uint8_t row);


static inline uint8_t
mcleds_disable (mcleds_t mcleds)
{
    return cleds_common_set (&mcleds->cleds, 0);
}


static inline void
mcleds_colourmap_set (mcleds_t mcleds, 
                      colourmap_t *colourmap, uint8_t colourmap_size)
{
    mcleds->colourmap = colourmap;
    mcleds->colourmap_size = colourmap_size;
}

#ifdef __cplusplus
}
#endif    
#endif


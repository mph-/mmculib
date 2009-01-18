/** @file   mcleds.c
    @author M. P. Hayes, UCECE
    @date   3 July 2007
    @brief 
*/

#include "mcleds.h"

mcleds_t
mcleds_init (mcleds_obj_t *mcleds, 
             const led_cfg_t *row_config, uint8_t rows_num,
             const led_cfg_t *col_config, uint8_t cols_num,
             colourmap_t *colourmap, uint8_t colourmap_size,
             mcleds_state_t *state, uint8_t update_rate)
{
    cleds_init (&mcleds->cleds,
                row_config, rows_num,
                col_config, cols_num);

    cleds_common_set (&mcleds->cleds, 0);

    mcleds->colourmap = colourmap;
    mcleds->colourmap_size = colourmap_size;

    mcleds->state = state;
    for (uint8_t i = 0; i < cols_num; i++)
        state[i].duty = 0;

    TICKER_INIT (&mcleds->primary_ticker, update_rate);

    return mcleds;
}


bool
mcleds_update (mcleds_t mcleds, uint8_t *screen)
{
    cleds_t cleds;
    uint8_t cols_num;
    colourmap_t *colourmap;
    mcleds_state_t *state;

    cleds = &mcleds->cleds;
    cols_num = cleds_cols_num_get (cleds);
    colourmap = mcleds->colourmap;
    state = mcleds->state;

    if (TICKER_UPDATE (&mcleds->primary_ticker))
    {
        uint8_t active_row;
        uint8_t primary;

        /* Switch all LED cathodes off until the anode states have
           been changed.  */    
        active_row = cleds_common_set (cleds, 0);

        primary = active_row - 1;
        active_row++;
        if (active_row > cleds_rows_num_get (cleds))
            active_row = 1;

        /* Set duty of LED anodes and turn active LED anodes on.  */
        for (uint8_t i = 0; i < cols_num; i++)
        {
            state[i].duty = colourmap[screen[i]][primary];
            cleds_set (cleds, i, state[i].duty != 0);
        }

        /* Switch next LED cathode on.  */      
        cleds_common_set (cleds, active_row);
        return 1;
    }
    else
    {
        /* Turn off LED anodes at end of duty.  */
        for (uint8_t i = 0; i < cols_num; i++)
        {
            if (state[i].duty && !--state[i].duty)
                cleds_set (cleds, i, 0);
        }
    }
    return 0;
}


void
mcleds_enable (mcleds_t mcleds, uint8_t row)
{
    cleds_t cleds;
    uint8_t cols_num;
    mcleds_state_t *state;

    cleds = &mcleds->cleds;
    cols_num = cleds_cols_num_get (cleds);
    state = mcleds->state;

    for (uint8_t i = 0; i < cols_num; i++)
        cleds_set (cleds, i, state[i].duty != 0);

    cleds_common_set (cleds, row);
}


void
mcleds_off (mcleds_t mcleds)
{
    uint8_t cols_num;

    cols_num = cleds_cols_num_get (&mcleds->cleds);

    for (uint8_t i = 0; i < cols_num; i++)
        cleds_set (&mcleds->cleds, i, 0);
}

/** @file   muxleds.c
    @author M. P. Hayes, UCECE
    @date   08 June 2002
    @brief 
*/

#define MUXLEDS_TRANSPARENT

#include "muxleds.h"

/* These routines are for controlling a multiplexed LED, piezoelectric
   tweeter, or any other 1 bit device configured in a matrix.  The
   implementation could be more efficient if we constrain the row and
   column lines to be contiguous.  */

/* By my definition, the columns are active low and the rows are
   active high.  */


void
muxleds_set (muxleds_t muxleds, uint8_t bit, uint8_t val)
{
    uint8_t row;
    uint8_t col;

    row = bit / muxleds->cols_num;
    col = bit % muxleds->cols_num;

    muxleds->cols[col].row_state |= BIT (row);
    if (val ^ muxleds->row_on)
        muxleds->cols[col].row_state ^= BIT (row);
}


void
muxleds_toggle (muxleds_t muxleds, uint8_t bit)
{
    uint8_t row;
    uint8_t col;

    row = bit / muxleds->cols_num;
    col = bit - row * muxleds->cols_num;

    muxleds->cols[col].row_state ^= BIT (row);
}


void
muxleds_update (muxleds_t muxleds)
{
    uint8_t i;
    uint8_t row_state;
    
    /* Disable col.  */
    port_pins_toggle (muxleds->cols[muxleds->col].port,
                      muxleds->cols[muxleds->col].bitmask);
    
    muxleds->col++;
    if (muxleds->col >= muxleds->cols_num)
        muxleds->col = 0;

    /* Drive the desired rows.  */
    row_state = muxleds->cols[muxleds->col].row_state;
    for (i = 0; i < muxleds->rows_num; i++)
    {
        portpins_set (muxleds->rows[i].port, muxleds->rows[i].bitmask,
                      row_state & 1);
        row_state >>= 1;
    }
    
    /* Enable col.  */
    port_pins_toggle (muxleds->cols[muxleds->col].port,
                      muxleds->cols[muxleds->col].bitmask);
}


/* Create a new MUXLEDS device.  */
muxleds_t
muxleds_init (muxleds_obj_t *muxleds,
              const muxleds_cfg_t *row_cfg,
              uint8_t rows_num,
              const muxleds_cfg_t *col_cfg,
              uint8_t cols_num,
              uint8_t row_on, uint8_t col_on)
{
    uint8_t i;

    for (i = 0; i < rows_num; i++)
    {
        muxleds->rows[i].port = row_cfg[i].port;
        muxleds->rows[i].bitmask = row_cfg[i].bitmask; 

        /* Configure port as output.  */
        port_pins_config_output (row_cfg[i].port, row_cfg[i].bitmask);

        /* Set output off.  */
        port_pins_set (muxleds->cols[i].port, 
                       muxleds->cols[i].bitmask, !row_on);
    }


    for (i = 0; i < cols_num; i++)
    {
        muxleds->cols[i].port = col_cfg[i].port;
        muxleds->cols[i].bitmask = col_cfg[i].bitmask; 

        /* Configure port as output.  */
        port_pins_config_output (col_cfg[i].port, col_cfg[i].bitmask);

        /* Set output off.  */
        portpins_set_high (muxleds->cols[i].port, 
                           muxleds->cols[i].bitmask, !col_on);

        muxleds->cols[i].row_state = 0;
    }

    muxleds->rows_num = rows_num;
    muxleds->cols_num = cols_num;
    muxleds->col = 0;
    muxleds->row_on = row_on;

    return muxleds;
}


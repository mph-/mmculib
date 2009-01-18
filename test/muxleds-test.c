#include <stdio.h>
#include "muxleds.h"

static const muxleds_cfg_t muxleds_row_cfg[] =
{
    {PORTA, 0},
    {PORTA, 1},
    {PORTA, 2}
};


static const muxleds_cfg_t muxleds_col_cfg[] =
{
    {PORTB, 0},
    {PORTB, 1},
    {PORTB, 2}
};



int main (void)
{
    muxleds_t muxleds;
    muxleds_dev_t muxleds_info;


    muxleds = muxleds_init (&muxleds_info, 
                            muxleds_row_cfg,
                            ARRAY_SIZE (muxleds_row_cfg),
                            muxleds_col_cfg,
                            ARRAY_SIZE (muxleds_col_cfg));

    muxleds_update (muxleds);


    return 0;
}

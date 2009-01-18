/** @file   biseq.c
    @author M. P. Hayes, UCECE
    @date   1 April 2007
    @brief  Bidirectional sequencer.
*/
#include <limits.h>
#include "biseq.h"


biseq_t biseq_init (biseq_obj_t *dev,
                int8_t (*callback) (void *data, char *str),
                void *callback_data)
{
    dev->callback = callback;
    dev->callback_data = callback_data;
    dev->step = 0;
    dev->str = 0;
    dev->dir = 1;
    dev->mode = BISEQ_MODE_NORMAL;
    return dev;
}


void biseq_set (biseq_t biseq, char *str)
{
    biseq_obj_t *dev = biseq;

    dev->str = str;
    dev->step = 0;
    dev->dir = 1;
}


char *biseq_get (biseq_t biseq)
{
    biseq_obj_t *dev = biseq;

    return dev->str;
}


void biseq_mode_set (biseq_t biseq, biseq_mode_t mode)
{
    biseq_obj_t *dev = biseq;

    dev->mode = mode;
    dev->step = 0;
    dev->dir = 1;
}


biseq_mode_t
biseq_mode_get (biseq_t biseq)
{
    biseq_obj_t *dev = biseq;

    return dev->mode;
}


int8_t biseq_update (biseq_t biseq)
{
    biseq_obj_t *dev = biseq;
    int8_t steps;

    if (!dev->str)
        return 0;

    steps = dev->callback (dev->callback_data, dev->str + dev->step);

    /* Update biseq state.  */
    if (dev->dir > 0)
    {
        /* Moving forward.  */
        dev->step += steps;

        if (!dev->str[dev->step])
        {
            if (dev->mode != BISEQ_MODE_CYCLE)
            {
                /* Reached end of sequence.  */
                dev->step = 0;
                return 1;
            }
            dev->dir = -1;
            dev->step -= 2;
        }
    }
    else
    {
        /* Moving backward.  */
        if (!dev->step)
        {
            dev->dir = 1;
            dev->step = 1;
            return 1;
        }
        else
        {
            dev->step -= steps;
        }
    }
    return 0;
}

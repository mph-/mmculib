/** @file   pacer.c
    @author M. P. Hayes, UCECE
    @date   23 August 2010
    @brief  Paced loop module
*/
#include "pit.h"
#include "pacer.h"

static pit_tick_t pacer_period;
static pit_tick_t pacer_when;


/** Initialise pacer:
    @param pacer_rate rate in Hz.  */
void pacer_init (pacer_rate_t pacer_rate)
{
    pit_init ();

    pacer_period = PIT_RATE / pacer_rate;
    pacer_when = pit_get ();
}


/** Wait until next pacer tick.  */
void pacer_wait (void)
{
    pit_wait_until (pacer_when);
    pacer_when += pacer_period;
}

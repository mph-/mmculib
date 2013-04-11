/** @file   pacer.c
    @author M. P. Hayes, UCECE
    @date   23 August 2010
    @brief  Paced loop module
*/
#include "pit.h"
#include "pacer.h"

static pit_tick_t pacer_period;


/** Initialise pacer:
    @param pacer_rate rate in Hz.  */
void pacer_init (pacer_rate_t pacer_rate)
{
    pit_init ();

    pacer_period = PIT_RATE / pacer_rate;
}


/** Wait until next pacer tick.  */
void pacer_wait (void)
{
    static pit_tick_t when = 0;

    pit_wait_until (when);
    when += pacer_period;
}

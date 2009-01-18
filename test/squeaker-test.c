#include <stdio.h>
#include "squeaker.h"


/* Define polling rates in Hz.  */
enum {FAST_LOOP_POLL_RATE = 20000};
enum {BEEP_POLL_RATE = FAST_LOOP_POLL_RATE};


enum {TUNE_BPM_RATE = 100};


squeaker_scale_t scale_table[] = MPWMTUNE_SCALE_TABLE (FAST_LOOP_POLL_RATE);


int
main (void)
{
    squeaker_dev_t squeaker_info;
    squeaker_t squeaker;
    int i;

    squeaker = squeaker_init (&squeaker_info, FAST_LOOP_POLL_RATE, scale_table);
    squeaker_speed_set (squeaker, TUNE_BPM_RATE);

    squeaker_set (squeaker, "CCEFDAAAAD+E+D+F+AAAA    ");

    i = 0;
    while (1)
    {
        squeaker_update (squeaker);
#if 0
        if (squeaker_update (squeaker))
            printf ("%3d: on\n", i);
        else
            printf ("%3d: off\n", i);       
#endif
        i++;
    }
}

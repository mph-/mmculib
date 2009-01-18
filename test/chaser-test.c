#include <stdio.h>
#include "chaser.h"
#include "flasher.h"

enum {LOOP_POLL_RATE = 5000};

static flasher_pattern_t flasher_pattern_off =
    {FLASHER_PATTERN (LOOP_POLL_RATE, 100, 50, 0.5, 100, 0, 0.5)};

static flasher_pattern_t flasher_pattern_on =
    {FLASHER_PATTERN (LOOP_POLL_RATE, 100, 50, 0.5, 100, 1, 0.5)};


font_t chaser_font =
{
    1, 0, 3,
    {0, 1, 2}
};


int main (void)
{
    chaser_t chaser;
    chaser_obj_t chaser_info;
    flasher_t flashers[1];
    flasher_obj_t flasher_info[1];
    int i;

    flashers[0] = flasher_init (&flasher_info[0]);

    chaser = chaser_init (&chaser_info, flashers, 1);

    chaser_patterns_set (chaser, 
                         &flasher_pattern_on,
                         &flasher_pattern_off);

    chaser_font_set (chaser, &chaser_font);

    chaser_sequence_set (chaser, "\1\2\2");

    chaser_mode_set (chaser, CHASER_MODE_INVERT);

    for (i = 0; i < 10; i++)
    {
        int seq_end;
        flasher_pattern_t *pattern;

        seq_end = chaser_update (chaser);

        pattern = flasher_pattern_get (flashers[0]);
        if (pattern == &flasher_pattern_on)
            printf ("%3d: on\n", i);
        else if (pattern == &flasher_pattern_off)
            printf ("%3d: off\n", i);
        else
            printf ("%3d: error\n", i);

    }


    chaser_mode_set (chaser, CHASER_MODE_NORMAL);

    for (i = 0; i < 10; i++)
    {
        int seq_end;
        flasher_pattern_t *pattern;

        seq_end = chaser_update (chaser);

        pattern = flasher_pattern_get (flashers[0]);
        if (pattern == &flasher_pattern_on)
            printf ("%3d: on\n", i);
        else if (pattern == &flasher_pattern_off)
            printf ("%3d: off\n", i);
        else
            printf ("%3d: error\n", i);

    }


    chaser_mode_set (chaser, CHASER_MODE_CYCLE);

    for (i = 0; i < 10; i++)
    {
        int seq_end;
        flasher_pattern_t *pattern;

        seq_end = chaser_update (chaser);

        pattern = flasher_pattern_get (flashers[0]);
        if (pattern == &flasher_pattern_on)
            printf ("%3d: on\n", i);
        else if (pattern == &flasher_pattern_off)
            printf ("%3d: off\n", i);
        else
            printf ("%3d: error\n", i);

    }
    return 0;
}

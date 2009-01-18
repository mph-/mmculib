/** @file   squeaker.c
    @author M. P. Hayes, UCECE
    @date   14 April 2007
    @brief  Play simple tunes with PWM.
*/

#define SQUEAKER_TRANSPARENT 1

#include "squeaker.h"


#define SQUEAKER_HOLDOFF_TIME 50e-3
enum {SQUEAKER_PRESCALER = 256};


/* By default notes are in the scale C4 -> C5.
   Note the first 12 frets on a six string guitar covers 3 octaves
   from the pitch E2 (82.41 Hz) to the pitch E5 (659.26 Hz).

   Tunes are specified using the notation C4C5F4G4C4 but for brevity
   this can be simplified to C4C5F4GC where the previously specified
   octave number persists.  A problem with this notation is the
   verbosity when we have something like B2C3B2C3.  Most melodies will
   only require 2 and 3 at the most octaves.  The ABC music notation
   uses CDEFGABcdefgabc'd'e'f'g'a'b'c' to denote 3 octaves from C2 to
   C5?  This uses numbers to indicate note duration, for example, C2
   denotes a C of twice the standard duration.

   For emphasis of first beat in bar, perhaps use ^ to indicate
   louder, for example, C^.  Similarly, to make a note quieter it
   could have a v suffix.  Alternatively, | bar markers could be
   inserted and perhaps time signatures.

   By default notes are quarter-notes.  It is probably best to define
   speed in terms of quarter-notes (beats per minute) rather than bars
   per minute since this requires bars to be defined.  Two bars in 4/4
   could be represented by C^DEDC^BAB whereas two bars in 3/4 could be
   represented by C^DEC^DE.
   
   So how should we denote note duration?  We need to distinguish
   between 2 identical quarter-notes (A A) played in succession and a
   half-note since this sounds different.

   We could use AA to indicate two A quarter-notes.  Alternatively, we
   could use AA to indicate an A half-note.  However, for a whole note
   we would need to indicate this with AAAA.  With the latter scheme
   we could separate two indentical quarter notes with a comma, for
   example, A,A.

   Rests are easy.  Each space represents one rest of quarter-note
   duration.  Two spaces represent a half-note rest.  Alternatively,
   we could represent this with " /".

   From a sequencing point of view it is simpler if every symbol
   represents a quarter-note rather than having variable length notes
   since this alters the sequencing timing.  This favours the approach
   of using AA to denote a half-note.

   If I implement a simple attack/decay response then it would be
   easier to use A/ for a half-note since we would interpret the / as
   to keep playing the previous note without sounding it again.
   Alternatively, when each new note is sounded there could be a short
   delay.

   >num could indicate jump forward to label num while <num could
   represent jump back to label num.  Although I prefer the notation
   <ABC>3 to represent playing the notes ABC in succession 3 times.
   This notation could be nested, for example, <ABC<DEF>2>3.
   Perhaps <ABC> denotes playing ABC indefinitely?  No I prefer
   a simple repeat.  Use ABC: for an infinite repeat.

   <ABC]1DE]2FG> represents ABCDEABCFG where ]n denotes alternate
   endings.  

*/



static void
squeaker_ticker_set (squeaker_t *squeaker)
{
    uint16_t speed;

    speed = squeaker->speed * squeaker->note_fraction;

#if 0
    /* The division is performed first to reduce the chance of integer
       overflow.  This results in poorer accuracy when the poll_rate
       is low.  In practice, the poll_rate cannot be too low otherwise
       the generated notes will be inaccurate.  */
    TICKER_INIT (&squeaker->ticker, (squeaker->poll_rate / speed) * 60);
#else
    TICKER_INIT (&squeaker->ticker, (squeaker->poll_rate / SQUEAKER_PRESCALER)
                 * 60 / speed);
#endif
}


static void 
squeaker_note_fraction_set (squeaker_t *squeaker, uint8_t note_fraction)
{
    squeaker->note_fraction = note_fraction;    
    squeaker_ticker_set (squeaker);
}


enum {SQUEAKER_SCALE_SIZE = 12};

void 
squeaker_note_set (squeaker_t *squeaker, 
                   squeaker_period_t period,
                   squeaker_period_t duty)
{
    squeaker->note_period = period;
    squeaker->note_duty = duty;
    squeaker->note_holdoff = squeaker->holdoff;
}


static void
squeaker_rest_play (squeaker_t *squeaker)
{
    squeaker_note_set (squeaker, 0, 0);
}


static void
squeaker_note_play (squeaker_t *squeaker, squeaker_note_t note)
{
    squeaker_period_t period;
    squeaker_period_t duty;
    uint8_t index;
    uint8_t octave;

#if 0
    /* See if we can play this note.  */
    if (note < SQUEAKER_NOTE_MIN)
        return;
#endif

    note -= SQUEAKER_NOTE_MIN;
    octave = note / SQUEAKER_SCALE_SIZE;
    index = note - octave * SQUEAKER_SCALE_SIZE;

    period = squeaker->scale_table[index];
    
    while (octave-- > 0)
        period /= 2;

    duty = period * squeaker->volume / 200;

    squeaker_note_set (squeaker, period, duty);

#if 0
    printf ("note = %d, octave = %d, period = %d\n",
            note, index / SQUEAKER_SCALE_SIZE, period);
#endif

}


static squeaker_note_t
squeaker_char_to_note (uint8_t ch)
{
    squeaker_note_t note;
    /* A = 9, B = 11, C = 0, D = 2, E = 4, F = 5, G = 7  */
    static const squeaker_note_t const lookup[] = {9, 11, 0, 2, 4, 5, 7};

    return lookup[ch - 'A'];

#if 0

    switch (ch)
    {
    default:
    case 'A':
    case 'B':
        note = (ch - 'A') * 2 + 9;
        break;

    case 'C':
    case 'D':
    case 'E':
        note = (ch - 'C') * 2;
        break;

    case 'F':
    case 'G':
        note = (ch - 'F') * 2 + 5;
        break;
    }
#endif

#if 0
    switch (ch)
    {
    case 'A':
        note = 9; break;
    case 'B':
        note = 11; break;
        break;
    case 'C':
        note = 0; break;
    case 'D':
        note = 2; break;
    case 'E':
        note = 4; break;
    case 'F':
        note = 5; break;
    case 'G':
        note = 7; break;
    default:
        note = 0;
        break;
    }

#endif

    return note;
}


/* Scan next part of melody until a note or end of melody is found.  */
static const char *
squeaker_scan (squeaker_t *squeaker, const char *str)
{
    while (1)
    {
        uint8_t num;
        char cmd;
        char modifier;
        bool have_hash;
        bool have_num;
        squeaker_note_t note;
        
        /* Play rest at end of melody.  */
        if (! *str)
        {
            squeaker_rest_play (squeaker);
            return str;
        }

        cmd = *str++;

        have_hash = *str == '#';
        if (have_hash)
            str++;

        modifier = 0;
        if (*str == '+' || *str == '-')
            modifier = *str++;

        have_num = 0;
        num = 0;
        while (*str >= '0' && *str <= '9')
        {
            have_num = 1;
            num = num * 10 + *str++ - '0';
        }

        switch (cmd)
        {
            /* Repeat sequence from start.  */
        case ':':
            str = squeaker->start;
            continue;

            /* Define start of loop.  */
        case '<':
            /* We could implement a small stack to allow nested
               loops.  */
            squeaker->loop_start = str;
            squeaker->loop_count = 0;
            continue;
            
            /* Loop.  */
        case '>':
            squeaker->loop_count++;
            if (!num)
                num = 2;

            if (squeaker->loop_count < num)
            {
                /* Jump to start of loop.  If no start of loop symbol,
                   jump to start.  */
                str = squeaker->loop_start;
                if (!str)
                    str = squeaker->start;
            }
            continue;

            /* Alternate endings.  */
        case '[':
            if (squeaker->loop_count == num - 1)
                continue;

            /* Skip to next alternate ending, the end of loop, or end of
               melody.  */
            while (*str && *str != '[' && *str != '>')
                str++;
            continue;

            /* Play rest.  */
        case ' ':
            squeaker_rest_play (squeaker);
            return str;
            break;
            
        case '*':
            if (num)
                squeaker_note_fraction_set (squeaker, num);
            continue;

        case '@':
            if (num)
                squeaker_speed_set (squeaker, num);
            continue;

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
            note = squeaker_char_to_note (cmd);

            if (have_hash)
                note++;

            if (have_num)
                squeaker->octave = num;

            if (modifier == '+')
                note += SQUEAKER_SCALE_SIZE;

            if (modifier == '-')
                note -= SQUEAKER_SCALE_SIZE;

            /* Convert note to MIDI note number.  */
            note += (squeaker->octave + 1) * SQUEAKER_SCALE_SIZE;

            squeaker_note_play (squeaker, note);
            return str;
            break;
            
            /* Continue with previous note.  */
        case '/':
        default:
            return str;
            break;
        }
    }
    return str;
}


void 
squeaker_play (squeaker_t *squeaker, const char *str)
{
    squeaker->cur = squeaker->start = str;
    squeaker->loop_start = 0;
    squeaker->loop_count = 0;
    squeaker->octave = SQUEAKER_OCTAVE_DEFAULT;
    squeaker_note_fraction_set (squeaker, 1);
}


/* Set (base) speed in beats per minute (BPM).  */
void 
squeaker_speed_set (squeaker_t *squeaker, squeaker_speed_t speed)
{
    squeaker->speed = speed;
    squeaker_ticker_set (squeaker);
}


/* Set volume as percentage of maximum.  */
void 
squeaker_volume_set (squeaker_t *squeaker, squeaker_volume_t volume)
{
    squeaker->volume = volume;
}


int8_t
squeaker_update (squeaker_t *squeaker)
{
    /* Rather than a 16 bit ticker it is faster to use
       an 8-bit prescaler and an 8-bit ticker.  */
    
    squeaker->prescaler++;
    if (! squeaker->prescaler)
    {
        if (TICKER_UPDATE (&squeaker->ticker))
        {
            if (squeaker->cur)
                squeaker->cur = squeaker_scan (squeaker, squeaker->cur);
        }

        /* We could halve the note duty after some interval to
           simulate note decay.  The decay interval could be related
           to the desired sustain.  This approach is a bit brutal
           especially when the duty is small.  */

        if (squeaker->note_holdoff)
            squeaker->note_holdoff--;
    }

    if (++squeaker->note_clock >= squeaker->note_period)
        squeaker->note_clock = 0;
    
    return squeaker->note_clock < squeaker->note_duty 
        && ! squeaker->note_holdoff;
}


squeaker_t
squeaker_init (squeaker_obj_t *squeaker, 
               uint16_t poll_rate,
               squeaker_scale_t *scale_table)
{
    squeaker->poll_rate = poll_rate;
    squeaker->scale_table = scale_table;
    squeaker->note_period = 0;
    squeaker->note_duty = 0;
    squeaker->volume = 100;
    squeaker->prescaler = 0;
    squeaker->holdoff = squeaker->poll_rate 
        / (uint16_t)(SQUEAKER_PRESCALER / SQUEAKER_HOLDOFF_TIME);

    squeaker_play (squeaker, 0);
    squeaker_speed_set (squeaker, SQUEAKER_SPEED_DEFAULT);
    return squeaker;
}

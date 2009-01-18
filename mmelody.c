/** @file   mmelody.c
    @author M. P. Hayes, UCECE
    @date   20 April 2007
    @brief  Play simple melodies.
*/

#define MMELODY_TRANSPARENT 1

#include "mmelody.h"


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

enum {MMELODY_SCALE_SIZE = 12};


static void
mmelody_ticker_set (mmelody_t mmelody)
{
    uint16_t speed;

    speed = mmelody->speed * mmelody->note_fraction;

    TICKER_INIT (&mmelody->ticker, mmelody->poll_rate * 60 * 4 / speed);
}


static void
mmelody_note_play (mmelody_t mmelody, mmelody_note_t note)
{
    mmelody->play_callback (mmelody->play_callback_data, note,
                            mmelody->volume);
}


/* Specify the default note length in fractions of a measure (bar).
   A value of 4 is the default which makes each note a quarter note.  */
static void 
mmelody_note_fraction_set (mmelody_t mmelody, uint8_t note_fraction)
{
    mmelody->note_fraction = note_fraction;    
    mmelody_ticker_set (mmelody);
}


static mmelody_note_t
mmelody_char_to_note (uint8_t ch)
{
    /* A = 9, B = 11, C = 0, D = 2, E = 4, F = 5, G = 7  */
    static const mmelody_note_t const lookup[] = {9, 11, 0, 2, 4, 5, 7};

    return lookup[ch - 'A'];
}


/* Scan next part of melody until a note or end of melody is found.  */
static const char *
mmelody_scan (mmelody_t mmelody, const char *str)
{
    while (1)
    {
        uint8_t num;
        char cmd;
        char modifier;
        bool have_hash;
        bool have_num;
        mmelody_note_t note;
        
        /* Play rest at end of melody.  */
        if (! *str)
        {
            mmelody_note_play (mmelody, 0);
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
            str = mmelody->start;
            continue;

            /* Define start of loop.  */
        case '<':
            /* We could implement a small stack to allow nested
               loops.  */
            mmelody->loop_start = str;
            mmelody->loop_count = 0;
            continue;
            
            /* Loop.  */
        case '>':
            mmelody->loop_count++;
            if (!num)
                num = 2;

            if (mmelody->loop_count < num)
            {
                /* Jump to start of loop.  If no start of loop symbol,
                   jump to start.  */
                str = mmelody->loop_start;
                if (!str)
                    str = mmelody->start;
            }
            continue;

            /* Alternate endings.  */
        case '[':
            if (mmelody->loop_count == num - 1)
                continue;

            /* Skip to next alternate ending, the end of loop, or end of
               melody.  */
            while (*str && *str != '[' && *str != '>')
                str++;
            continue;

            /* Play rest.  */
        case ' ':
            mmelody_note_play (mmelody, 0);
            return str;
            break;
            
        case '*':
            if (num)
                mmelody_note_fraction_set (mmelody, num);
            continue;

        case '@':
            if (num)
                mmelody_speed_set (mmelody, num);
            continue;

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
            note = mmelody_char_to_note (cmd);

            if (have_hash)
                note++;

            if (have_num)
                mmelody->octave = num;

            if (modifier == '+')
                note += MMELODY_SCALE_SIZE;

            if (modifier == '-')
                note -= MMELODY_SCALE_SIZE;

            /* Convert note to MIDI note number.  */
            note += (mmelody->octave + 1) * MMELODY_SCALE_SIZE;

            mmelody_note_play (mmelody, note);
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
mmelody_play (mmelody_t mmelody, const char *str)
{
    mmelody->cur = mmelody->start = str;
    mmelody->loop_start = 0;
    mmelody->loop_count = 0;
    mmelody->octave = MMELODY_OCTAVE_DEFAULT;
    mmelody_note_fraction_set (mmelody, 4);
}


/* Set (base) speed in beats per minute (BPM).  */
void 
mmelody_speed_set (mmelody_t mmelody, mmelody_speed_t speed)
{
    mmelody->speed = speed;
    mmelody_ticker_set (mmelody);
}


/* Set volume as percentage of maximum.  */
void 
mmelody_volume_set (mmelody_t mmelody, mmelody_volume_t volume)
{
    mmelody->volume = volume;
}


void
mmelody_update (mmelody_t mmelody)
{
    if (TICKER_UPDATE (&mmelody->ticker))
    {
        if (mmelody->cur)
            mmelody->cur = mmelody_scan (mmelody, mmelody->cur);
    }
}


mmelody_t
mmelody_init (mmelody_obj_t *mmelody, 
              uint16_t poll_rate,
              mmelody_callback_t play_callback,
              void *play_callback_data)
{
    mmelody->poll_rate = poll_rate;
    mmelody->play_callback = play_callback;
    mmelody->play_callback_data = play_callback_data;
    mmelody->volume = 100;
    mmelody->note_fraction = 1;
    mmelody_speed_set (mmelody, MMELODY_SPEED_DEFAULT);
    mmelody_play (mmelody, 0);
    return mmelody;
}

#include <stdlib.h>

#include "bits.h"
#include "spi_pga.h"

/* The MAX9939 requires data to be sent LSB first (ignoring the
   contradictory diagram in the datasheet) but most SPI peripherals
   send data MSB first.  In this driver, the data is swapped around.

   The CS setup time is 80 ns.
*/

enum 
{
    MAX9939_SHDN = BIT (0),
    MAX9939_MEAS = BIT (1),
    MAX9939_NEG = BIT (2),
    MAX9939_GAIN = BIT (7)
};


/* The minimum gain is 0.2 for Vcc = 5 V or 0.25 for Vcc = 3.3 V.
   Let's assume 3.3 V operation and scale all the gains by 8.  */
#define GAIN_SCALE(GAIN) ((GAIN) * 8)

static const spi_pga_gain_t max9939_gains[] =
{
    GAIN_SCALE (0.25),
    GAIN_SCALE (1),
    GAIN_SCALE (10),
    GAIN_SCALE (20),
    GAIN_SCALE (30),
    GAIN_SCALE (40),
    GAIN_SCALE (60),
    GAIN_SCALE (80),
    GAIN_SCALE (120),
    GAIN_SCALE (157),
    /* Terminate list with a zero gain.  */
    0
};


#define GAIN_COMMAND(REGVAL) ((REGVAL))

static const uint8_t gain_commands[] =
{
    GAIN_COMMAND (0xc8),
    GAIN_COMMAND (0x80),
    GAIN_COMMAND (0xc0),
    GAIN_COMMAND (0xa0),
    GAIN_COMMAND (0xe0),
    GAIN_COMMAND (0x90),
    GAIN_COMMAND (0xd0),
    GAIN_COMMAND (0xb0),
    GAIN_COMMAND (0xf0),
    GAIN_COMMAND (0x88)
};


typedef struct
{
    uint16_t offset;
    uint8_t regval;
} max9939_offset_map_t;


#define OFFSET_MAP(OFFSET, REGVAL) {.offset = (OFFSET) * 10, \
            .regval = ((REGVAL) << 3)}

static const max9939_offset_map_t max9939_offset_map[] =
{
    /* Scale the offsets to tenths of a millivolt
       and flip the bit order since LSB first.  */
    OFFSET_MAP (0.0, 0x0),
    OFFSET_MAP (1.3, 0x8),
    OFFSET_MAP (2.5, 0x4),
    OFFSET_MAP (3.8, 0xc),
    OFFSET_MAP (4.9, 0x2),
    OFFSET_MAP (6.1, 0xa),
    OFFSET_MAP (7.3, 0x6),
    OFFSET_MAP (8.4, 0xe),
    OFFSET_MAP (10.6, 0x1),
    OFFSET_MAP (11.7, 0x9),
    OFFSET_MAP (12.7, 0x5),
    OFFSET_MAP (13.7, 0xd),
    OFFSET_MAP (14.7, 0x3),
    OFFSET_MAP (15.7, 0xb),
    OFFSET_MAP (16.7, 0x7),
    OFFSET_MAP (17.6, 0xf)
};


static bool
max9939_gain_set (spi_pga_t pga, uint8_t gain_index)
{
    uint8_t command[1];
    
#if 0
    printf ("gain %u\n", max9939_gains[gain_index]);
#endif

    command[0] = gain_commands[gain_index];
    
    return spi_pga_command (pga, command, ARRAY_SIZE (command));
}


static spi_pga_offset_t
max9939_offset_set1 (spi_pga_t pga, uint8_t index, bool negative, bool measure)
{
    spi_pga_offset_t offset;
    uint8_t command[1];
    
    offset = max9939_offset_map[index].offset;
    
    command[0] = max9939_offset_map[index].regval;

    if (negative)
    {
        offset = -offset;
        command[0] |= MAX9939_NEG;
    }

    if (measure)
        command[0] |= MAX9939_MEAS;

#if 0
    printf ("off %d, gain %d\n", offset, max9939_gains[pga->gain_index]);
#endif
    
    if (!spi_pga_command (pga, command, ARRAY_SIZE (command)))
        return 0;

    pga->offset_index = index;
    return offset;
}


/* Setting a positive offset (in 0.1 mV steps) makes the output drop.  */
static spi_pga_offset_t
max9939_offset_set (spi_pga_t pga, spi_pga_offset_t offset, bool measure)
{
    unsigned int i;
    bool negative;

    /* Need to measure offset voltage at low(ish) gains otherwise will
       have saturation.  For example, the worst case correction is
       17.1 mV and with the maximum gain of 628 this produces 10 V of
       offset.  Thus the maximum gain to avoid saturation is 80.  Now
       it appears that the offset also varies with gain but this is
       probably a secondary effect. 
       
       The output offset voltage is a function of the PGA gain G_1 and
       the output amplifier gain G_2.  When the input is shorted the
       output voltage is (from the datasheet)

       Vo = ((Voff1 + Voffc) G1 + Voff2) G2 + Vcc / 2,
          = (Voff1 + Voffc) G1 G_2 + Voff2 G2 + Vcc / 2,

       where Voffc is the programmed offset voltage correction and
       Voff1 and Voff2 are the inherent offset voltages for the two
       amplifiers.  Both the amplifiers have a similar offset voltage:
       1.5 mV typ and 9 mV max at 25 degrees.  This includes the
       effects of mismatches in ther internal Vcc/2 resistor dividers
       that bias the output.  At high gains when G1 >> G2, the effect
       of Voff2 can be neglected.

       It appears that the offset correction is opposite to what appears
       in the datasheet.  It appears that

       Vo = (Voff1 - Voffc) G1 G_2 + Voff2 G2 + Vcc / 2.

       Thus increasing Voffc causes the output to drop.

    */

    negative = offset < 0;
    if (negative)
        offset = -offset;

    /* Searching for bracketing range.  */
    for (i = 1; i < ARRAY_SIZE (max9939_offset_map); i++)
    {
        if (offset < max9939_offset_map[i].offset)
            break;
    }

    if (i != ARRAY_SIZE (max9939_offset_map))
    {
        /* Choose closest value.  */
        if (abs (offset - max9939_offset_map[i].offset)
            < abs (offset - max9939_offset_map[i - 1].offset))
            i++;
    }
    
    return max9939_offset_set1 (pga, i - 1, negative, measure);
}


static bool 
max9939_shutdown_set (spi_pga_t pga, bool enable)
{
    uint8_t command[1];

    if (enable)
        command[0]= 0;
    else
        command[0] = MAX9939_SHDN;

    if (!spi_pga_command (pga, command, ARRAY_SIZE (command)))
                return 0;
    return 1;
}



static bool 
max9939_input_short_set (spi_pga_t pga, bool enable)
{
    uint8_t command[1];

    command[0] = max9939_offset_map[pga->offset_index].regval;

    if (enable)
        command[0] |= MAX9939_MEAS;

    if (!spi_pga_command (pga, command, ARRAY_SIZE (command)))
        return 0;
    return 1;
}


spi_pga_ops_t max9939_ops =
{
    .gain_set = max9939_gain_set,   
    .channel_set = 0,
    .offset_set = max9939_offset_set,   
    .input_short_set = max9939_input_short_set,   
    .shutdown_set = max9939_shutdown_set,   
    .gains = max9939_gains
};



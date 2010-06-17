#include "bits.h"
#include "spi_pga.h"

/* The MAX9930 requires data to be sent LSB first (ignoring the
   contradictory diagram in the datasheet) but most SPI peripeherals
   send data MSB first.  In this driver, the data is swapped around.
*/

enum 
{
    MAX9930_SHDN = BIT (0),
    MAX9930_MEAS = BIT (1),
    MAX9930_GAIN = BIT (7)
};


typedef struct
{
    uint16_t gain;
    uint8_t regval;
} max9939_gain_map_t;


/* The minimum gain is 0.2 for Vcc = 5 V or 0.25 for Vcc = 3.3 V.
   Let's assume 3.3 V operation and scale all the gains by 4.  */
#define GAIN_MAP(GAIN, REGVAL) {.gain = (GAIN) * 4, \
            .regval = ((REGVAL) >> 1) | MAX9930_GAIN}

static max9939_gain_map_t gain_map[] =
{
    GAIN_MAP (0.25, 0x90),
    GAIN_MAP (1, 0x00),
    GAIN_MAP (10, 0x80),
    GAIN_MAP (20, 0x40),
    GAIN_MAP (30, 0xc0),
    GAIN_MAP (40, 0x20),
    GAIN_MAP (60, 0xa0),
    GAIN_MAP (80, 0x60),
    GAIN_MAP (120, 0xe0),
    GAIN_MAP (157, 0x80)
};



static spi_pga_gain_t
max9939_gain_set1 (spi_pga_t pga, uint index)
{
    spi_pga_gain_t gain;
    uint8_t command[1];
    
    gain = gain_map[index].gain;
    
    command[0] = gain_map[index].regval;
    
    if (!spi_pga_command (pga, command, ARRAY_SIZE (command)))
        return 0;
    
    return gain;
}


/* Set the desired gain or the next lowest if unavailable.  This will
   wakeup the PGA from shutdown.  */
static spi_pga_gain_t
max9939_gain_set (spi_pga_t pga, spi_pga_gain_t gain)
{
    unsigned int i;
    uint16_t prev_gain;

    /* TODO:  Ponder.  */
    if (gain == 0)
        return 0;

    prev_gain = 0;
    for (i = 0; i < ARRAY_SIZE (gain_map); i++)
    {
        if (gain >= prev_gain && gain < gain_map[i].gain)
            return max9939_gain_set1 (pga, i - 1);
        prev_gain = gain_map[i].gain;
    }
    
    return max9939_gain_set1 (pga, i - 1);
}


static spi_pga_offset_t
max9939_offset_set (spi_pga_t pga, spi_pga_offset_t offset, bool enable)
{

    return 0;
}


static bool 
max9939_shutdown_set (spi_pga_t pga, bool enable)
{
    uint8_t command[1];

    if (enable)
        command[0]= 0;
    else
        command[0] = MAX9930_SHDN;

    if (!spi_pga_command (pga, command, ARRAY_SIZE (command)))
                return 0;
    return 1;
}


spi_pga_ops_t max9939_ops =
{
    .gain_set = max9939_gain_set,   
    .channel_set = 0,
    .offset_set = max9939_offset_set,   
    .shutdown_set = max9939_shutdown_set,   
};



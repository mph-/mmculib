#include "spi_pga.h"

/* The minimum gain is 0.2 for Vcc = 5 V or 0.25 for Vcc = 3.3 V.
   Let's assume 3.3 V operation and scale all the gains by 4.  */
#define MAX9939_GAINS {1, 1 * 4, 10 * 4, 20 * 4, 30 * 4, 40 * 4, 60 * 4, 80 * 4, 120 * 4, 157 * 4}

static uint16_t gains[] = MAX9939_GAINS;

/* This will wakeup the PGA from shutdown.  */
static spi_pga_gain_t
max9939_gain_set (spi_pga_t pga, spi_pga_gain_t gain)
{
    unsigned int i;
    uint16_t prev_gain;

    prev_gain = 0;
    for (i = 0; i < ARRAY_SIZE (gains); i++)
    {
        if ((i == ARRAY_SIZE (gains) - 1)
            || (gain > prev_gain && gain <= gains[i]))
        {
            uint8_t command[1];

            gain = gains[i];

            if (i == 0)
                i = 0x08;
            else
                i--;

            command[0] = (i << 1) | 0x01;

            if (!spi_pga_command (pga, command, ARRAY_SIZE (command)))
                return 0;

            return gain;
        }
        prev_gain = gains[i];
    }

    return 0;
}


static spi_pga_offset_t
max9939_offset_set (spi_pga_t pga, spi_pga_offset_t offset, bool enable)
{

    return 0;
}


static bool 
max9939_shutdown_set (spi_pga_t pga, bool enable)
{

    return 0;
}


spi_pga_ops_t max9939_ops =
{
    .gain_set = max9939_gain_set,   
    .channel_set = 0,
    .offset_set = max9939_offset_set,   
    .shutdown_set = max9939_shutdown_set,   
};



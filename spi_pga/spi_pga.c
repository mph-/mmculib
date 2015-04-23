#include "spi_pga_dev.h"
#include "spi_pga.h"

#include "max9939.h"
#include "mcp6s2x.h"


#ifndef SPI_PGA_DEVICES_NUM
#define SPI_PGA_DEVICES_NUM 4
#endif 


static uint8_t spi_pga_devices_num = 0;
static spi_pga_dev_t spi_pga_devices[SPI_PGA_DEVICES_NUM];


bool
spi_pga_command (spi_pga_t pga, uint8_t *commands, uint8_t len)
{
    return spi_write (pga->spi, commands, len, 1) == len;
}


spi_pga_t
spi_pga_init (const spi_pga_cfg_t *cfg)
{
    spi_pga_dev_t *spi_pga;

    if (spi_pga_devices_num >= SPI_PGA_DEVICES_NUM)
        return 0;

    spi_pga = spi_pga_devices + spi_pga_devices_num;

    
    /* Initialise spi port.  */
    spi_pga->spi = spi_init (&cfg->spi);
    if (!spi_pga->spi)
        return 0;

    spi_cs_setup_set (spi_pga->spi, 4);

    switch (cfg->type)
    {
    case SPI_PGA_MAX9939:
        spi_pga->ops = &max9939_ops;
        /* Each data transfer is a single byte so the CS framing
           is not important.  */
        break;

    case SPI_PGA_MCP6S21:
    case SPI_PGA_MCP6S2X:
        spi_pga->ops = &mcp6s2x_ops;
        /* The CS needs to be get low for the two 8 bit transfers.  */
        spi_cs_mode_set (spi_pga->spi, SPI_CS_MODE_FRAME);
        break;

    default:
        return 0;
    }

    spi_pga_devices_num++;
    return spi_pga;
}


static void
spi_pga_gain_index_set (spi_pga_t pga, uint8_t gain_index)
{
    pga->ops->gain_set (pga, gain_index);
    pga->gain_index = gain_index;
}


spi_pga_gain_t 
spi_pga_gain_set (spi_pga_t pga, spi_pga_gain_t gain)
{
    unsigned int i;
    const uint16_t *gains;
    uint8_t gain_index;

    gains = pga->ops->gains;

    /* The specified gain is considered a maximum so we need to choose
       the next lower value if no gains match.  */
    for (i = 1; gains[i] && (gain >= gains[i]); i++)
        continue;

    gain_index = i - 1;
        
    spi_pga_gain_index_set (pga, gain_index);
    return gains[gain_index];
}


spi_pga_gain_t 
spi_pga_gain_get (spi_pga_t pga)
{
    const uint16_t *gains;

    gains = pga->ops->gains;
    
    return gains[pga->gain_index];
}


spi_pga_gain_t 
spi_pga_gain_next_get (spi_pga_t pga)
{
    const uint16_t *gains;

    gains = pga->ops->gains;
    if (gains[pga->gain_index])
        pga->gain_index++;
    
    return gains[pga->gain_index];
}


spi_pga_channel_t
spi_pga_channel_set (spi_pga_t pga, spi_pga_channel_t channel)
{
    if (!pga->ops->channel_set)
        return 0;
    pga->channel = pga->ops->channel_set (pga, channel);
    return pga->channel;
}


spi_pga_offset_t
spi_pga_offset_set (spi_pga_t pga, spi_pga_offset_t offset, bool measure)
{
    if (!pga->ops->offset_set)
        return 0;
    return pga->ops->offset_set (pga, offset, measure);
}


bool
spi_pga_input_short_set (spi_pga_t pga, bool enable)
{
    if (!pga->ops->input_short_set)
        return 0;
    return pga->ops->input_short_set (pga, enable);
}


bool
spi_pga_shutdown (spi_pga_t pga)
{
    if (!pga->ops->shutdown_set)
        return 0;
    if (!pga->ops->shutdown_set (pga, 1))
        return 0;

    spi_shutdown (pga->spi);
    return 1;
}


bool
spi_pga_wakeup (spi_pga_t pga)
{
    if (!pga->ops->shutdown_set)
        return 0;

    // spi_wakeup ?  */
    return pga->ops->shutdown_set (pga, 0);
}

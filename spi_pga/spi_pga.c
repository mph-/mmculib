#include "spi_pga_dev.h"
#include "spi_pga.h"

#include "max9939.h"

//#ifdef SPI_PGA_MAX9939
//#endif


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

    switch (cfg->type)
    {
    case SPI_PGA_MAX9939:
        spi_pga->ops = &max9939_ops;
        break;

    default:
        return 0;
    }

    spi_pga_devices_num++;
    return spi_pga;
}


spi_pga_gain_t 
spi_pga_gain_set (spi_pga_t pga, spi_pga_gain_t gain)
{
    pga->gain = pga->ops->gain_set (pga, gain);
    return pga->gain;
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
spi_pga_offset_set (spi_pga_t pga, spi_pga_offset_t offset, bool enable)
{
    if (!pga->ops->offset_set)
        return 0;
    pga->offset = pga->ops->offset_set (pga, offset, enable);
    return pga->offset;
}


bool
spi_pga_shutdown (spi_pga_t pga)
{
    if (!pga->ops->shutdown_set)
        return 0;
    return pga->ops->shutdown_set (pga, 1);
}


bool
spi_pga_wakeup (spi_pga_t pga)
{
    if (!pga->ops->shutdown_set)
        return 0;
    return pga->ops->shutdown_set (pga, 0);
}

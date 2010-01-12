#include "config.h"
#include "msd.h"
#include "spi_dataflash.h"

#include <string.h>

#ifndef SPI_DATAFLASH_PAGE_SIZE
#define SPI_DATAFLASH_PAGE_SIZE 264
#endif

#ifndef SPI_DATAFLASH_PAGES
#define SPI_DATAFLASH_PAGES 2048
#endif


static msd_size_t
dataflash_msd_read (void *dev, msd_addr_t addr, void *buffer, msd_size_t size)
{
    return spi_dataflash_read (dev, addr, buffer, size);
}


static msd_size_t
dataflash_msd_write (void *dev, msd_addr_t addr, const void *buffer,
                     msd_size_t size)
{
    return spi_dataflash_write (dev, addr, buffer, size);
}


static msd_status_t
dataflash_msd_status_get (void *dev)
{
    return MSD_STATUS_READY;
}


static msd_t dataflash_msd =
{
    .handle = 0,
    .read = dataflash_msd_read,
    .write = dataflash_msd_write,
    .status_get = dataflash_msd_status_get,
    .media_bytes = SPI_DATAFLASH_PAGE_SIZE * SPI_DATAFLASH_PAGES,
    .block_bytes = SPI_DATAFLASH_PAGE_SIZE,
    .flags.removable = 0,
    .name = "DATAFLASH_MSD"
};


static const spi_dataflash_cfg_t dataflash_cfg =
{
    .spi.channel = SPI_DATAFLASH_SPI_CHANNEL,
    .spi.clock_divisor =  F_CPU / 20e6 + 1,
    .spi.cs = SPI_DATAFLASH_CS,
    .wp = SPI_DATAFLASH_WP,
    .pages = SPI_DATAFLASH_PAGES,
    .page_size = SPI_DATAFLASH_PAGE_SIZE,
};


static spi_dataflash_obj_t dataflash_obj;


msd_t *
dataflash_msd_init (void)
{
    dataflash_msd.handle = spi_dataflash_init (&dataflash_obj, &dataflash_cfg);
    if (!dataflash_msd.handle)
        return NULL;

    /* The number of pages and page size should be probed...  */

    return &dataflash_msd;
}

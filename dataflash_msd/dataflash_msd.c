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


#ifndef SPI_DATAFLASH_SECTOR_SIZE
/* The actual page is 264 but defining as 256 is more efficient
   when using the flash as a mass storage device since blocks
   are usually multiples of 256 bytes.  */
#define SPI_DATAFLASH_SECTOR_SIZE 256
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
dataflash_msd_status_get (void *dev __unused__)
{
    return MSD_STATUS_READY;
}


static void
dataflash_msd_shutdown (void *dev)
{
    return spi_dataflash_shutdown (dev);
}


static const msd_ops_t dataflash_msd_ops =
{
    .read = dataflash_msd_read,
    .write = dataflash_msd_write,
    .status_get = dataflash_msd_status_get,
    .shutdown = dataflash_msd_shutdown,
};


static msd_t dataflash_msd =
{
    .handle = 0,
    .ops = &dataflash_msd_ops,
    .media_bytes = SPI_DATAFLASH_SECTOR_SIZE * SPI_DATAFLASH_PAGES,
    .block_bytes = SPI_DATAFLASH_SECTOR_SIZE,
    .flags = {.removable = 0, .partial_read = 1, .partial_write = 1},
    .name = "Dataflash"
};


static const spi_dataflash_cfg_t dataflash_cfg =
{
    .spi = {.channel = SPI_DATAFLASH_SPI_CHANNEL,
            .clock_divisor =  F_CPU / 20e6 + 1,
            .cs = SPI_DATAFLASH_CS,
            .mode = SPI_MODE_0,
            .bits = 8},
    .wp = SPI_DATAFLASH_WP,
    .pages = SPI_DATAFLASH_PAGES,
    .page_size = SPI_DATAFLASH_PAGE_SIZE,
    .sector_size = SPI_DATAFLASH_SECTOR_SIZE
};


msd_t *
dataflash_msd_init (void)
{
    dataflash_msd.handle = spi_dataflash_init (&dataflash_cfg);
    if (!dataflash_msd.handle)
        return NULL;

    /* The number of pages and page size should be probed...  */

    return &dataflash_msd;
}

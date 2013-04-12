#include "config.h"
#include "delay.h"
#include "pio.h"
#include "spi.h"
#include "spi_dataflash.h"

/* The AT45DB041D uses SPI modes 0 and 3.  The page size is
   configurable 256/264 bytes.  Parts are usually shipped with the
   page size set to 264 bytes but can be reconfigured to 256 bytes.
   The additional 8 bytes are usually used for error detection and
   correction.  Reconfiguring can only be done once and is only
   necessary for special applications.  You are better off just
   ignoring the additional 8 bytes at the end of each page.  All
   program operations are in terms of pages.  A page, block (2 KB),
   sector (64 KB), or entire chip can be erased.  A sector is 256
   pages while a block is 8 pages.  It has two internal SRAM buffers
   that can be used for holding a page each so that external memory is
   not required when programming.

   The AT45DB041B does not have the newer READ_CONT_SLOW and READ_CONT_FAST
   commands.

   The chip select setup time is 250 ns (before the first rising clock
   edge) and the chip select hold time is 250 ns (after the last
   falling clock edge).

*/


typedef uint16_t spi_dataflash_offset_t;
typedef uint16_t spi_dataflash_page_t;


#define SPI_DATAFLASH_OP_READ_CONT SPI_DATAFLASH_OP_READ_CONT_LEGACY

enum {SPI_DATAFLASH_OP_READ_CONT_SLOW = 0x03,  /* Read data from memory.  */
      SPI_DATAFLASH_OP_READ_CONT_FAST = 0x0B,  /* Read data from memory.  */
      SPI_DATAFLASH_OP_READ_CONT_LEGACY = 0xE8, /* Read data from memory.  */
      SPI_DATAFLASH_OP_BLOCK_ERASE = 0x50,      /* Erase block.  */
      SPI_DATAFLASH_OP_TRANSFER_BUFFER1 = 0x53, /* Fill buffer1 from memory.  */
      SPI_DATAFLASH_OP_TRANSFER_BUFFER2 = 0x55, /* Fill buffer2 from memory.  */
      SPI_DATAFLASH_OP_COMPARE_BUFFER1 = 0x60, /* Compare buffer1.  */
      SPI_DATAFLASH_OP_COMPARE_BUFFER2 = 0x61, /* Compare buffer2.  */
      SPI_DATAFLASH_OP_SECTOR_ERASE = 0x7C,    /* Erase sector.  */
      SPI_DATAFLASH_OP_PAGE_ERASE = 0x81,      /* Erase page.  */
      SPI_DATAFLASH_OP_WRITE_PROGRAM_BUFFER1 = 0x82, /* Program buffer1.  */
      SPI_DATAFLASH_OP_PROGRAM_BUFFER1 = 0x83, /* Program buffer1.  */
      SPI_DATAFLASH_OP_WRITE_BUFFER1 = 0x84,   /* Write buffer1.  */
      SPI_DATAFLASH_OP_WRITE_PROGRAM_BUFFER2 = 0x85, /* Program buffer1.  */
      SPI_DATAFLASH_OP_PROGRAM_BUFFER2 = 0x86, /* Program buffer2.  */
      SPI_DATAFLASH_OP_WRITE_BUFFER2 = 0x87,   /* Write buffer2.  */
      SPI_DATAFLASH_OP_ID_READ = 0x9F,         /* Read chip ID.  */
      SPI_DATAFLASH_OP_WAKEUP = 0xAB,          /* Wake from deep power down.  */
      SPI_DATAFLASH_OP_POWERDOWN = 0xB9,       /* Deep power down.  */
      SPI_DATAFLASH_OP_CHIP_ERASE = 0xC7,      /* Erase chip.  */
      SPI_DATAFLASH_OP_READ_BUFFER1_SLOW = 0xD1,  /* Read buffer1.  */
      SPI_DATAFLASH_OP_READ_BUFFER1_FAST = 0xD4,  /* Read buffer2.  */
      SPI_DATAFLASH_OP_READ_BUFFER2_SLOW = 0xD3,  /* Read buffer1.  */
      SPI_DATAFLASH_OP_READ_BUFFER2_FAST = 0xD6,   /* Read buffer2.  */
      SPI_DATAFLASH_OP_STATUS_READ = 0x0D7     /* Read Status Register.  */
};


enum {SPI_DATAFLASH_STATUS_RDY = BIT (7),
      SPI_DATAFLASH_STATUS_NOT_MATCH = BIT (6),
      SPI_DATAFLASH_STATUS_PROTECT = BIT (1),
      SPI_DATAFLASH_STATUS_SIZE = BIT (0)};


#define SPI_DATAFLASH_RETRIES 50


#ifndef SPI_DATAFLASH_DEVICES_NUM
#define SPI_DATAFLASH_DEVICES_NUM 4
#endif 


static uint8_t spi_dataflash_devices_num = 0;
static spi_dataflash_dev_t spi_dataflash_devices[SPI_DATAFLASH_DEVICES_NUM];


uint8_t
spi_dataflash_status_read (spi_dataflash_t dev)
{
    uint8_t command[2];

    command[0] = SPI_DATAFLASH_OP_STATUS_READ;

    spi_transfer (dev->spi, command, command, sizeof (command), 1);
    
    return command[1];
}


static bool
spi_dataflash_ready_wait (spi_dataflash_t dev)
{
    int i;

    /* Operations take between 5--20 ms.  */
    for (i = 0; i < SPI_DATAFLASH_RETRIES; i++)
    {
        if (spi_dataflash_status_read (dev) & SPI_DATAFLASH_STATUS_RDY)
            return 1;
        delay_ms (1);
    }

    return 0;
}


spi_dataflash_ret_t
spi_dataflash_read (spi_dataflash_t dev, spi_dataflash_addr_t addr,
                    void *buffer, spi_dataflash_size_t total_bytes)
{
    uint8_t command[8];
    uint16_t sector_size;
    spi_dataflash_offset_t offset;
    spi_dataflash_size_t readlen;
    spi_dataflash_size_t read_bytes;
    spi_dataflash_page_t page;
    uint8_t *dst;

    if (!total_bytes)
        return 0;
    if (addr + total_bytes > dev->size)
        return -1;

    /* Check that powered and ready.  */
    if (!spi_dataflash_ready_wait (dev))
        return 0;

    dst = buffer;

    sector_size = dev->cfg->sector_size;
    page = addr / sector_size;
    offset = addr % sector_size;

    if (offset + total_bytes > sector_size)
        readlen = sector_size - offset;
    else
        readlen = total_bytes;

    read_bytes = 0;
    while (read_bytes < total_bytes) 
    {
        spi_dataflash_offset_t remaining_bytes;

        /* Remap address into page address + offset.  */
        addr = (page << dev->page_bits) + offset;        

        /* Set up for continuous memory read.  */
        command[0] = SPI_DATAFLASH_OP_READ_CONT;
        command[1] = (addr >> 16) & 0xff;
        command[2] = (addr >> 8) & 0xff;
        command[3] = addr & 0xff;
        /* The next 4 bytes are dummy don't care bytes.  */

        spi_write (dev->spi, command, sizeof (command), 0);

        spi_read (dev->spi, dst, readlen, 1);
        dst += readlen;

        page++;
        offset = 0;
        read_bytes += readlen;

        remaining_bytes = total_bytes - read_bytes;
        
        if (remaining_bytes > sector_size)
            readlen = sector_size;
        else
            readlen = remaining_bytes;
    }
    return total_bytes;
}


/** Read from dataflash using a scatter approach to a vector of
    descriptors.  */
spi_dataflash_ret_t
spi_dataflash_readv (spi_dataflash_t dev, spi_dataflash_addr_t addr,
                     iovec_t *iov, iovec_count_t iov_count)
{
    unsigned int i;
    iovec_size_t size;

    size = 0;
    for (i = 0; i < iov_count; i++)
    {
        /* FIXME for error handling.  */
        spi_dataflash_read (dev, addr, iov[i].data, iov[i].len);
        size += iov[i].len;
        addr += iov[i].len;
    }
    return size;
}


/** Write to dataflash using a gather approach from a vector of
    descriptors.  The idea is to coalesce writes to the dataflash
    to minimise the number of erase operations.  */
spi_dataflash_ret_t
spi_dataflash_writev (spi_dataflash_t dev, spi_dataflash_addr_t addr,
                      iovec_t *iov, iovec_count_t iov_count)
{
    spi_dataflash_page_t page;
    spi_dataflash_offset_t offset;
    spi_dataflash_size_t writelen;
    spi_dataflash_size_t written_bytes;
    const uint8_t *src;
    uint16_t sector_size;
    spi_dataflash_size_t total_bytes;
    spi_dataflash_size_t vlen;
    int iov_num;
    unsigned int i;

    /* Determine total number of bytes to write.  */
    total_bytes = 0;
    for (i = 0; i < iov_count; i++)
        total_bytes += iov[i].len;

    if (!total_bytes)
        return 0;
    if (addr + total_bytes > dev->size)
        return -1;

    if (dev->cfg->wp)
        pio_output_high (dev->cfg->wp);

    sector_size = dev->cfg->sector_size;
    page = addr / sector_size;
    offset = addr % sector_size;

    if (offset + total_bytes > sector_size)
        writelen = sector_size - offset;
    else
        writelen = total_bytes;
    
    src = 0;
    iov_num = 0;
    vlen = 0;
    written_bytes = 0;
    while (written_bytes < total_bytes) 
    {
        spi_dataflash_offset_t remaining_bytes;
        spi_dataflash_size_t wlen;
        uint8_t command[4];

        addr = page << dev->page_bits;

        /* If not programming a full page then need to read
           partial buffer.  */
        if (writelen != sector_size) 
        {
            command[0] = SPI_DATAFLASH_OP_TRANSFER_BUFFER1;
            command[1] = (addr >> 16) & 0xff;
            command[2] = (addr >> 8) & 0xff;
            command[3] = 0;

            spi_write (dev->spi, command, 4, 1);
            
            if (!spi_dataflash_ready_wait (dev))
                break;
        }

        addr += offset;
        command[0] = SPI_DATAFLASH_OP_WRITE_PROGRAM_BUFFER1;
        command[1] = (addr >> 16) & 0xff;
        command[2] = (addr >> 8) & 0xff;
        command[3] = addr & 0xff;

        spi_write (dev->spi, command, 4, 0);

        wlen = writelen;
        while (wlen)
        {
            spi_dataflash_size_t slen;

            if (!vlen)
            {
                src = iov[iov_num].data;
                vlen = iov[iov_num].len;
                iov_num++;
            }

            slen = wlen;
            if (slen > vlen)
                slen = vlen;
            
            spi_write (dev->spi, src, slen, wlen == slen);
            src += slen;
            wlen -= slen;
            vlen -= slen;
        }

        if (!spi_dataflash_ready_wait (dev))
            return written_bytes;

        addr = page << dev->page_bits;
        command[0] = SPI_DATAFLASH_OP_COMPARE_BUFFER1;
        command[1] = (addr >> 16) & 0xff;
        command[2] = (addr >> 8) & 0xff;
        command[3] = 0;
        
        spi_write (dev->spi, command, 4, 1);

        if (!spi_dataflash_ready_wait (dev))
            break;

        /* Check if compare failed.  */
        if (spi_dataflash_status_read (dev) & SPI_DATAFLASH_STATUS_NOT_MATCH)
            break;
        
        page++;
        offset = 0;
        written_bytes += writelen;

        remaining_bytes = total_bytes - written_bytes;
        
        if (remaining_bytes > sector_size)
            writelen = sector_size;
        else
            writelen = remaining_bytes;
    }

    if (dev->cfg->wp)
        pio_output_low (dev->cfg->wp);

    return written_bytes;
}


spi_dataflash_ret_t
spi_dataflash_write (spi_dataflash_t dev, spi_dataflash_addr_t addr,
                     const void *buffer, spi_dataflash_size_t len)
{
    iovec_t iov;

    iov.data = (void *)buffer;
    iov.len = len;
    
    return spi_dataflash_writev (dev, addr, &iov, 1);
}


spi_dataflash_t
spi_dataflash_init (const spi_dataflash_cfg_t *cfg)
{
    spi_dataflash_t dev;
    uint16_t page_size;

    if (spi_dataflash_devices_num >= SPI_DATAFLASH_DEVICES_NUM)
        return 0;

    dev = spi_dataflash_devices + spi_dataflash_devices_num;

    page_size = cfg->page_size;
    dev->cfg = cfg;
    dev->page_bits = 0;

    while (page_size)
    {
        page_size >>= 1;
        dev->page_bits++;
    }

    /* A sector is smaller than equal to the size of the page.
       Usually a sector is a power of 2; the additional bytes in the
       page can be used for a checksum.  */

    if (cfg->sector_size > cfg->page_size)
        return 0;

    dev->size = cfg->pages * cfg->sector_size;

    dev->spi = spi_init (&cfg->spi);
    spi_mode_set (dev->spi, SPI_MODE_0);
    spi_cs_mode_set (dev->spi, SPI_CS_MODE_FRAME);
    /* Ensure chip select isn't asserted too soon.  */
    spi_cs_assert_delay_set (dev->spi, 16);    
    /* Ensure chip select isn't negated too soon.  */
    spi_cs_negate_delay_set (dev->spi, 16);    

    if (cfg->wp)
        pio_config_set (cfg->wp, PIO_OUTPUT_LOW);

    spi_dataflash_status_read (dev);

    return dev;
}


void
spi_dataflash_shutdown (spi_dataflash_t dev)
{
    uint8_t command[1];

    command[0] = SPI_DATAFLASH_OP_POWERDOWN;

    spi_write (dev->spi, command, sizeof (command), 1);
    spi_shutdown (dev->spi);
}


void
spi_dataflash_wakeup (spi_dataflash_t dev)
{
    uint8_t command[1];

    command[0] = SPI_DATAFLASH_OP_WAKEUP;

    spi_write (dev->spi, command, sizeof (command), 1);
}


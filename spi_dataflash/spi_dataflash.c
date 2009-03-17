#include "config.h"
#include "delay.h"
#include "port.h"
#include "spi.h"
#include "spi_dataflash.h"

/* The AT45DB041D uses SPI modes 0 and 3.  The page size is
   configurable 256/264 bytes.  Parts are usually shipped with the
   page size set to 264 bytes but can be reconfigured to 256 bytes.
   The additional 8 bytes are usually used for error detection and
   correction.  Reconfiguring can only be done once.  All program
   operations are in terms of pages.  A page, block (2 KB), sector (64
   KB), or entire chip can be erased.  A sector is 256 pages while a
   block is 8 pages.  It has two internal SRAM buffers that can be
   used for holding a page each so that external memory is not
   required when programming.

   The AT45DB041B does not have the newer READ_CONT_SLOW and READ_CONT_FAST
   commands.

   The chip select setup time is 250 ns (before the first rising clock
   edge) and the chip select hold time is 250 ns (after the last
   falling clock edge).

*/


typedef uint8_t spi_dataflash_offset_t;
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
                    void *buffer, spi_dataflash_size_t len)
{
    uint8_t command[8];
    uint16_t page_size;

    if (!len)
        return 0;
    if (addr + len > dev->size)
        return -1;

    page_size = dev->cfg->page_size;

    /* Remap address into page address + offset.  */
    addr = ((addr / page_size) << dev->page_bits)
        + addr % page_size;

    /* Set up for continuous memory read.  */
    command[0] = SPI_DATAFLASH_OP_READ_CONT;
    command[1] = (addr >> 16) & 0xff;
    command[2] = (addr >> 8) & 0xff;
    command[3] = addr & 0xff;
    /* The next 4 bytes are dummy don't care bytes.  */

    spi_write (dev->spi, command, sizeof (command), 0);

    return spi_read (dev->spi, buffer, len, 1);
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
    descriptors.  The idea is to coalesce writes to ther dataflash
    to minimise the number of erase operations.  */
spi_dataflash_ret_t
spi_dataflash_writev (spi_dataflash_t dev, spi_dataflash_addr_t addr,
                      iovec_t *iov, iovec_count_t iov_count)
{
    spi_dataflash_page_t page;
    spi_dataflash_offset_t offset;
    spi_dataflash_offset_t writelen;
    spi_dataflash_size_t bytes_written;
    const uint8_t *data;
    uint16_t page_size;
    spi_dataflash_size_t len;
    spi_dataflash_size_t vlen;
    int iovnum;
    unsigned int i;

    /* Determine total number of bytes to write.  */
    len = 0;
    for (i = 0; i < iov_count; i++)
        len += iov[i].len;

    if (!len)
        return 0;
    if (addr + len > dev->size)
        return -1;

    if (dev->cfg->wp.port)
        port_pins_set_high (dev->cfg->wp.port, dev->cfg->wp.bitmask);

    page_size = dev->cfg->page_size;
    page = addr / page_size;
    offset = addr % page_size;

    if (offset + len > page_size)
        writelen = page_size - offset;
    else
        writelen = len;
    
    data = 0;
    iovnum = 0;
    vlen = 0;
    bytes_written = 0;
    while (bytes_written < len) 
    {
        spi_dataflash_offset_t bytes_left;
        spi_dataflash_size_t wlen;
        uint8_t command[4];

        addr = page << dev->page_bits;

        /* If not programming a full page then need to read
           partial buffer.  */
        if (writelen != page_size) 
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
                data = iov[iovnum].data;
                vlen = iov[iovnum].len;
                iovnum++;
            }

            slen = wlen;
            if (slen > vlen)
                slen = vlen;
            
            spi_write (dev->spi, data, slen, 1);
            data += slen;
            wlen -= slen;
            vlen -= slen;
        }

        if (!spi_dataflash_ready_wait (dev))
            return bytes_written;

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
        bytes_written += writelen;

        bytes_left = len - writelen;
        
        if (bytes_left > page_size)
            writelen = page_size;
        else
            writelen = bytes_left;
    }

    if (dev->cfg->wp.port)
        port_pins_set_low (dev->cfg->wp.port, dev->cfg->wp.bitmask);

    return bytes_written;
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
spi_dataflash_init (spi_dataflash_obj_t *obj, const spi_dataflash_cfg_t *cfg)
{
    spi_dataflash_t dev;
    uint16_t page_size;

    dev = obj;

    page_size = cfg->page_size;
    dev->cfg = cfg;
    dev->page_bits = 0;

    while (page_size)
    {
        page_size >>= 1;
        dev->page_bits++;
    }

    dev->size = cfg->pages * cfg->page_size;

    dev->spi = spi_init (&cfg->spi);
    spi_mode_set (dev->spi, SPI_MODE_0);
    spi_cs_mode_set (dev->spi, SPI_CS_MODE_FRAME);
    /* Ensure chip select isn't asserted too soon.  */
    spi_cs_assert_delay_set (dev->spi, 16);    
    /* Ensure chip select isn't negated too soon.  */
    spi_cs_negate_delay_set (dev->spi, 16);    

    if (cfg->wp.port)
    {
        port_pins_config_output (cfg->wp.port, cfg->wp.bitmask);
        port_pins_set_low (cfg->wp.port, cfg->wp.bitmask);
    }

    spi_dataflash_status_read (dev);

    return dev;
}


void
spi_dataflash_shutdown (spi_dataflash_t dev)
{
    uint8_t command[1];

    command[0] = SPI_DATAFLASH_OP_POWERDOWN;

    spi_write (dev->spi, command, sizeof (command), 1);
}


void
spi_dataflash_wakeup (spi_dataflash_t dev)
{
    uint8_t command[1];

    command[0] = SPI_DATAFLASH_OP_WAKEUP;

    spi_write (dev->spi, command, sizeof (command), 1);
}


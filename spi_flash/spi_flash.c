#include "config.h"
#include "delay.h"
#include "port.h"
#include "spi.h"
#include "spi_flash.h"

/* The AT45DB041D uses SPI modes 0 and 3.  The page size is
   configurable 256/264 bytes.  Parts are usually shipped with the
   page size set to 264 bytes but can be reconfigured to 256 bytes.
   Reconfiguring can only be done once.  All program operations are in
   terms of pages.  A page, block (2 KB), sector (64 KB), or entire
   chip can be erased.  A sector is 256 pages while a block is 8
   pages.  It has two internal SRAM buffers that can be used for
   holding a page each so that external memory is not required when
   programming.


   With this driver we use 264 byte pages but only store 256 bytes per
   page.  The other 8 bytes could be used at a later date.  This means
   we cannot easily use the continuous array read operations but need
   to read a page at a time, extracting the bytes of interest.

*/


typedef uint8_t spi_flash_offset_t;
typedef uint16_t spi_flash_page_t;


#ifndef SPI_FLASH_SPI_CHANNEL
#define SPI_FLASH_SPI_CHANNEL 0
#endif


/* The page size is what we pretend.  */
#define SPI_FLASH_PAGE_SIZE	256
#define SPI_FLASH_BLOCK_SIZE	2048
#define SPI_FLASH_SECTOR_SIZE	0x10000
#define	SPI_FLASH_SECTORS_NUM 	128


enum {SPI_FLASH_OP_WRSR = 1,  /* Write Status Register.  */
      SPI_FLASH_OP_PP = 2,    /* Program Data into memory.  */
      SPI_FLASH_OP_READ_SLOW = 3,  /* Read data from memory.  */
      SPI_FLASH_OP_READ_FAST = 0x0B,  /* Read data from memory.  */
      SPI_FLASH_OP_WRDI = 4,  /* Reset Write Enable Latch.  */
      SPI_FLASH_OP_RDSR = 5,  /* Read Status Register.  */
      SPI_FLASH_OP_WREN = 6,  /* Set Write Enable Latch.  */
      SPI_FLASH_OP_ID_READ = 0x9F,  /* Read chip ID.  */
      SPI_FLASH_OP_FILL_BUFFER1 = 0x53,  /* Fill buffer1 from memory.  */
      SPI_FLASH_OP_FILL_BUFFER2 = 0x55,  /* Fill buffer1 from memory.  */
      SPI_FLASH_OP_READ_BUFFER1_SLOW = 0xD1,  /* Read buffer1.  */
      SPI_FLASH_OP_READ_BUFFER1_FAST = 0xD4,  /* Read buffer2.  */
      SPI_FLASH_OP_READ_BUFFER2_SLOW = 0xD3,  /* Read buffer1.  */
      SPI_FLASH_OP_READ_BUFFER2_FAST = 0xD6,  /* Read buffer2.  */
      SPI_FLASH_OP_WRITE_BUFFER1 = 0x84,  /* Write buffer1.  */
      SPI_FLASH_OP_WRITE_BUFFER2 = 0x87,  /* Write buffer2.  */
      SPI_FLASH_OP_PAGE_ERASE = 0x81,    /* Erase page.  */
      SPI_FLASH_OP_SECTOR_ERASE = 0x7C,  /* Erase sector.  */
      SPI_FLASH_OP_BLOCK_ERASE = 0x50,   /* Erase block.  */
      SPI_FLASH_OP_CHIP_ERASE = 0xC7,    /* Erase chip.  */
      SPI_FLASH_OP_POWERDOWN = 0xB9,     /* Deep power down.  */
      SPI_FLASH_OP_WAKEUP = 0xAB         /* Wake from deep power down.  */
};


enum {SPI_FLASH_STATUS_WEL = BIT (1),
      SPI_FLASH_STATUS_WIP = BIT (0)};


#define SPI_FLASH_RETRIES 1000


#define SPI_FLASH_PUTC(val) spi_putc(val)
#define SPI_FLASH_GETC() spi_getc()

#define SPI_FLASH_OP(op) SPI_FLASH_PUTC (op)


/* \CS low.  */
#define SPI_FLASH_SELECT() port_pins_set_low (SPI_FLASH_CS_PORT, SPI_FLASH_CS_BIT)

/* \CS high.  */
#define SPI_FLASH_DESELECT() port_pins_set_high (SPI_FLASH_CS_PORT, SPI_FLASH_CS_BIT)


static uint8_t
spi_flash_status_read (void)
{
    uint8_t status;

    SPI_FLASH_SELECT ();

    SPI_FLASH_OP (SPI_FLASH_OP_RDSR);

    /* Read back contents of status register.  */
    status = SPI_FLASH_GETC ();
    
    /* Deselect device.  */
    SPI_FLASH_DESELECT ();
    
    return status;
}


static bool
spi_flash_wel_wait (void)
{
    int i;

    for (i = 0; i < SPI_FLASH_RETRIES; i++)
    {
        if (spi_flash_status_read () & SPI_FLASH_STATUS_WEL)
            return 1;
        DELAY_US (1);
    }

    return 0;
}


static bool
spi_flash_WIP_wait (void)
{
    int i;

    for (i = 0; i < SPI_FLASH_RETRIES; i++)
    {
        if (spi_flash_status_read () & SPI_FLASH_STATUS_WIP)
            return 1;
        DELAY_US (1);
    }
    return 0;
}


static inline void
spi_flash_address_set (spi_flash_addr_t addr)
{
    SPI_FLASH_PUTC (addr >> 16);
    SPI_FLASH_PUTC ((addr >> 8) & 0xff);
    SPI_FLASH_PUTC (addr & 0xff);
}


static inline spi_flash_addr_t
spi_flash_address_map (spi_flash_addr_t addr)
{
    spi_flash_page_t page;
    spi_flash_offset_t offset;

    /* This mapping assumes that the device has 264 byte pages
       and that we are only using 256 bytes per page.  */

    page = addr >> 8;
    offset = addr & 0xff;
    
    return (page << 9) | offset;
}


static inline spi_flash_page_t
spi_flash_page_get (spi_flash_addr_t addr)
{
    return addr >> 8;
}


static inline spi_flash_offset_t
spi_flash_offset_get (spi_flash_addr_t addr)
{
    return addr & 0xff;
}


static inline spi_flash_size_t
spi_flash_page_size_get (spi_flash_addr_t addr)
{
    spi_flash_offset_t offset;

    offset = spi_flash_offset_get (addr);

    return SPI_FLASH_PAGE_SIZE - offset;
}


static spi_flash_ret_t
spi_flash_sector_number_get (spi_flash_addr_t addr)
{
    if (addr > (SPI_FLASH_SECTORS_NUM * SPI_FLASH_SECTOR_SIZE - 1))
        return -SPI_FLASH_SECTOR_INVALID;

    return (int)addr / SPI_FLASH_SECTOR_SIZE; 
}


static spi_flash_ret_t
spi_flash_block_erase (int block)
{
    spi_flash_addr_t addr;

    if (block < 0 || block > SPI_FLASH_SECTORS_NUM)
        return -SPI_FLASH_BLOCK_INVALID;

    addr = block * SPI_FLASH_SECTOR_SIZE;


    SPI_FLASH_SELECT ();
    SPI_FLASH_OP (SPI_FLASH_OP_WREN);
    SPI_FLASH_DESELECT ();

    if (!spi_flash_wel_wait ())
        return -SPI_FLASH_TIMEOUT;

    /* Select device.  */
    SPI_FLASH_SELECT ();
    
    /* Request to read.  */
    SPI_FLASH_OP (SPI_FLASH_OP_BLOCK_ERASE);
    
    /* Send 24 bit address, high byte first.  */
    SPI_FLASH_PUTC (addr >> 16);
    SPI_FLASH_PUTC ((addr >> 8) & 0xff);
    SPI_FLASH_PUTC (addr & 0xff);

    SPI_FLASH_DESELECT ();

    /* Wait for sector erase to complete.  */
    if (!spi_flash_WIP_wait ())
        return -SPI_FLASH_TIMEOUT;

    return SPI_FLASH_OK;
}


static spi_flash_ret_t
spi_flash_page_write (spi_flash_addr_t addr, const uint8_t *buffer)
{
    int i;

    SPI_FLASH_SELECT ();
    SPI_FLASH_OP (SPI_FLASH_OP_WREN);
    SPI_FLASH_DESELECT ();

    if (!spi_flash_wel_wait ())
        return -SPI_FLASH_TIMEOUT;

    /* Select device.  */
    SPI_FLASH_SELECT ();
    
    /* Request to read.  */
    SPI_FLASH_OP (SPI_FLASH_OP_PP);

    /* Send 24 bit address, high byte first.  */    
    SPI_FLASH_PUTC (addr >> 16);
    SPI_FLASH_PUTC ((addr >> 8) & 0xff);
    SPI_FLASH_PUTC (addr & 0xff);

    for (i = 0; i < SPI_FLASH_PAGE_SIZE; i++) 
        SPI_FLASH_PUTC (*buffer++);

    SPI_FLASH_DESELECT ();

    if (!spi_flash_WIP_wait ())
        return -SPI_FLASH_TIMEOUT;

    return SPI_FLASH_PAGE_SIZE;
}


static spi_flash_ret_t
spi_flash_page_read (spi_flash_addr_t addr, uint8_t *buffer, 
                     spi_flash_size_t size)
{
    int i;
    spi_flash_size_t len;

    /* Select device.  */
    SPI_FLASH_SELECT ();
    
    /* Request to read.  */
    SPI_FLASH_OP (SPI_FLASH_OP_READ_PAGE);

    spi_flash_address_set (spi_flash_address_map (addr));

    /* We can only read to the end of a page before we wrap around.  */
    len = spi_flash_page_size_get (addr);
    if (size > len)
        size = len;

    /* Need 4 dummy reads.  */
    for (i = 0; i < 4; i++) 
        SPI_FLASH_GETC ()

    for (i = 0; i < size; i++) 
        *buffer++ = SPI_FLASH_GETC ()

    SPI_FLASH_DESELECT ();

    return size;
}


spi_flash_ret_t
spi_flash_read (spi_flash_addr_t addr, void *buffer, spi_flash_size_t size)
{
    uint8_t *data;
    spi_flash_size_t bytes;

    spi_channel_select (SPI_FLASH_SPI_CHANNEL);

    data = buffer;

    bytes = size;
    while (bytes)
    {
        spi_flash_size_t len;

        len = spi_flash_page_read (addr, data, bytes);
        addr += len;
        bytes -= len;
        data += len;
    }

    spi_channel_deselect (SPI_FLASH_SPI_CHANNEL);
    return size;
}


spi_flash_ret_t
spi_flash_write (spi_flash_addr_t addr, const void *buffer,
                 spi_flash_size_t size)
{
    spi_flash_ret_t ret;
    int	start_block;
    int end_block;
    spi_flash_size_t start_addr;
    spi_flash_size_t end_addr;
    spi_flash_size_t i;
    const uint8_t *data;
    int block;
    /* Yikes, this is big.  */
    uint8_t temp[SPI_FLASH_SECTOR_SIZE];

    data = buffer;

    spi_channel_select (SPI_FLASH_SPI_CHANNEL);

    /* Get the start block number.  */
    ret = spi_flash_sector_number_get (addr);
    if (ret < 0)
        return ret;
    start_block = ret;

    /* Get the end block number.  */
    ret = spi_flash_sector_number_get (addr + size - 1);
    if (ret < 0)
        return ret;
    end_block = ret;

    for (block = start_block; block <= end_block; block++)
    {
        /* Read sector.  */
        start_addr = block * SPI_FLASH_SECTOR_SIZE;
        spi_flash_read1 (start_addr, temp, SPI_FLASH_SECTOR_SIZE);

        end_addr = (block + 1) * SPI_FLASH_SECTOR_SIZE - 1;
        if (start_addr < addr)
            start_addr = addr;
        if (end_addr >= (addr + size))
            end_addr = (addr + size - 1);

        /* Modify sector.  */
        for (i = start_addr; i <= end_addr; i++)
            temp[i - block * SPI_FLASH_SECTOR_SIZE] = data[i - addr];

        /* Rewrite sector.  */
        spi_flash_block_erase (block);
        ret = spi_flash_sector_write (block * SPI_FLASH_SECTOR_SIZE, temp);
        if (ret < 0)
            return ret;
    }

    spi_channel_deselect (SPI_FLASH_SPI_CHANNEL);
    return size;
}


void
spi_flash_init (void)
{
#ifdef SPI_FLASH_WP_PORT
    port_pin_config_output (SPI_FLASH_WP_PORT, SPI_FLASH_WP_BIT);
#endif

    /* Make chip select pin an output.  */
    port_pin_config_output (SPI_FLASH_CS_PORT, SPI_FLASH_CS_BIT);
    
    /* Initialise spi port.  */
    spi_init ();
}



void
spi_flash_shutdown (void)
{
    spi_channel_select (SPI_FLASH_SPI_CHANNEL);

    /* Select device.  */
    SPI_FLASH_SELECT ();
    
    /* Request to read.  */
    SPI_FLASH_OP (SPI_FLASH_POWERDOWN);

    SPI_FLASH_DESELECT ();

    spi_channel_deselect (SPI_FLASH_SPI_CHANNEL);
}



void
spi_flash_wakeup (void)
{
    spi_channel_select (SPI_FLASH_SPI_CHANNEL);

    /* Select device.  */
    SPI_FLASH_SELECT ();
    
    /* Request to read.  */
    SPI_FLASH_OP (SPI_FLASH_WAKEUP);

    SPI_FLASH_DESELECT ();

    spi_channel_deselect (SPI_FLASH_SPI_CHANNEL);

    /* We need to wait 35 microseconds before accessing device.  */
}


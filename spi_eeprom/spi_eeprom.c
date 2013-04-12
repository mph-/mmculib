/** @file s_eeprom.c
 *  @author Michael Hayes
 *  @date 6 August 2003
 * 
 *  @brief Routines to read/write SPI EEPROM.
 */

#define SPI_EEPROM_TRANSPARENT
 
#include "s_eeprom.h"
#include "spi.h"
#include "delay.h"

#include <stdio.h>

/* Device	  Size	Page Size
   25XX160	 0x800	       16
   25XX640	0x2000         32
   25XX256      0x8000    0x8000
*/



/* EEPROM operations.  */
enum {SPI_EEPROM_OP_WRSR = 1,   /* Write status register.  */
      SPI_EEPROM_OP_WRITE = 2,  /* Write byte.  */
      SPI_EEPROM_OP_READ = 3,   /* Read byte.  */
      SPI_EEPROM_OP_WRDI = 4,   /* Write disable.  */
      SPI_EEPROM_OP_RDSR = 5,   /* Read status register.  */
      SPI_EEPROM_OP_WREN = 6};  /* Write enable.  */


/* STATUS register bits.  */
enum {SPI_EEPROM_STATUS_WPEN = BIT (7),
      SPI_EEPROM_STATUS_BP1 = BIT (3),
      SPI_EEPROM_STATUS_BP0 = BIT (2),
      SPI_EEPROM_STATUS_WEL = BIT (1),
      SPI_EEPROM_STATUS_WIP = BIT (0)};


enum {SPI_EEPROM_RETRIES = 1000};


/* Write status register.  */
static void 
spi_eeprom_status_write (spi_eeprom_t eeprom, uint8_t data)
{
    uint8_t command[2];

    command[0] = SPI_EEPROM_OP_WRSR;
    command[1] = data;

    spi_write (eeprom->spi, command, 2, 1);            
}


static uint8_t
spi_eeprom_status_read (spi_eeprom_t dev)
{
    uint8_t command[2];

    command[0] = SPI_EEPROM_OP_RDSR;

    spi_transfer (dev->spi, command, command, sizeof (command), 1);
    
    return command[1];
}


static bool
spi_eeprom_wip_wait (spi_eeprom_t dev)
{
    int i;

    for (i = 0; i < SPI_EEPROM_RETRIES; i++)
    {
        if (spi_eeprom_status_read (dev) & SPI_EEPROM_STATUS_WIP)
            return 1;
        DELAY_US (1);
    }

    return 0;
}


/* Read LEN bytes from ADDR into BUFFER.  */
spi_eeprom_size_t
spi_eeprom_read (spi_eeprom_t eeprom, spi_eeprom_addr_t addr,
                 void *buffer, spi_eeprom_size_t len)
{
    uint8_t command[3];

    if ((addr + len) > eeprom->cfg->size)
        return 0;

    command[0] = SPI_EEPROM_OP_READ;
    command[1] = addr >> 8;
    command[2] = addr & 0xff;
            
    spi_write (eeprom->spi, command, 3, 0);            
    
    return spi_read (eeprom->spi, buffer, len, 1);
}


/* Write LEN bytes to ADDR from BUFFER.  */
spi_eeprom_size_t
spi_eeprom_write (spi_eeprom_t eeprom, spi_eeprom_addr_t addr,
                  const void *buffer, spi_eeprom_size_t len)
{
    const uint8_t *data;
    spi_eeprom_size_t bytes_written;

    if ((addr + len) > eeprom->cfg->size)
        return 0;

    /* Maybe disable write protect here.  */

    data = buffer;
    bytes_written = 0;
    while (bytes_written < len) 
    {
        uint8_t command[3];
        spi_eeprom_size_t offset;
        spi_eeprom_size_t writelen;
        spi_eeprom_size_t bytes_left;

        command[0] = SPI_EEPROM_OP_WREN;
        spi_write (eeprom->spi, command, 1, 1);            

        /* Ensure CS high for 500 ns.  */
        DELAY_US (1);

        command[0] = SPI_EEPROM_OP_WRITE;
        command[1] = addr >> 8;
        command[2] = addr & 0xff;
        spi_write (eeprom->spi, command, 3, 0);            

        /* Send bytes until enough sent or end of page reached.  */
        offset = addr % eeprom->cfg->page_size;
        writelen = eeprom->cfg->page_size - offset;

        bytes_left = len - bytes_written;
        if (bytes_left < writelen)
            writelen = bytes_left;
        
        writelen = spi_write (eeprom->spi, data, writelen, 1); 
        addr += writelen;
        data += writelen;
        bytes_written += writelen;
        
        /* At end of write, the write enable flag is cleared. 
           Need to ensure CS high for 500 ns.  */
        DELAY_US (1);

        if (!spi_eeprom_wip_wait (eeprom))
            return bytes_written;
    }

    /* Maybe write protect here.  */

    return bytes_written;
}


/* Setup SPI EEPROM for continuous writing.  */
uint8_t
spi_eeprom_write_setup (spi_eeprom_t eeprom, spi_eeprom_addr_t addr)
{
    uint8_t command[3];

    command[0] = SPI_EEPROM_OP_WREN;
    spi_write (eeprom->spi, command, 1, 1);            
    
    /* Ensure CS high for 500 ns.  */
    DELAY_US (1);
    
    command[0] = SPI_EEPROM_OP_WRITE;
    command[1] = addr >> 8;
    command[2] = addr & 0xff;
    spi_write (eeprom->spi, command, 3, 0);            

    return 1;
}


/* Initialise the SPI EEPROM for operation.  INFO is a pointer into
   RAM that stores the state of the SPI EEPROM.  CFG is a pointer into
   ROM to define the port the SPI EEPROM is connected to.  The
   returned handle is passed to the other spi_eeprom_xxx routines to
   denote the SPI EEPROM to operate on.  */
spi_eeprom_t
spi_eeprom_init (spi_eeprom_obj_t *eeprom, const spi_eeprom_cfg_t *cfg)
{
    if (cfg->wp)
    {
        pio_config_set(cfg->wp, PIO_OUTPUT_HIGH);
    }

    eeprom->cfg = cfg;

    /* Initialise spi port.  */
    eeprom->spi = spi_init (&cfg->spi);
    spi_mode_set (eeprom->spi, SPI_MODE_0);
    spi_cs_mode_set (eeprom->spi, SPI_CS_MODE_FRAME);
    
    /* Disable write protect and block protect.  */
    spi_eeprom_status_write (eeprom, 0);

    return eeprom;
}


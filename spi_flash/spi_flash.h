/** @file spi_flash.h
 *  @author Michael Hayes
 *  @date 18 May 2008
 * 
 *  @brief Routines to communicate with a SPI FLASH memory.
 */
 
#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include "config.h"
#include "port.h"


typedef uint32_t spi_flash_addr_t;
typedef uint32_t spi_flash_size_t;
typedef int32_t spi_flash_ret_t;


typedef enum
{
    SPI_FLASH_OK,
    SPI_FLASH_TIMEOUT,
    SPI_FLASH_SECTOR_INVALID,
    SPI_FLASH_BLOCK_INVALID,
} spi_flash_err_t;


extern void
spi_flash_init (void);


/* Read SIZE bytes from ADDR into BUFFER.  */
extern spi_flash_ret_t
spi_flash_read (spi_flash_addr_t addr, void *buffer, spi_flash_size_t size);


/* Write SIZE bytes to ADDR from BUFFER.  */
extern spi_flash_ret_t 
spi_flash_write (spi_flash_addr_t addr, const void *buffer,
                 spi_flash_size_t size);

#endif

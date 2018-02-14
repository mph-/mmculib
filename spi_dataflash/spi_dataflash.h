/** @file spi_dataflash.h
 *  @author Michael Hayes
 *  @date 06/08/03
 * 
 *  @brief Routines to communicate with a SPI DATAFLASH.
 */
 
#ifndef SPI_DATAFLASH_H
#define SPI_DATAFLASH_H

#ifdef __cplusplus
extern "C" {
#endif
    


#include "config.h"
#include "iovec.h"
#include "pio.h"
#include "spi.h"


typedef struct
{
    spi_cfg_t spi;
    pio_t wp;
    uint16_t pages;
    uint16_t page_size;
    uint16_t sector_size;
} spi_dataflash_cfg_t;    


typedef struct
{
    spi_t spi;
    uint8_t page_bits;
    uint32_t size;
    const spi_dataflash_cfg_t *cfg;
} spi_dataflash_dev_t;



typedef spi_dataflash_dev_t *spi_dataflash_t;

typedef enum
{
    SPI_FLASH_OK,
    SPI_FLASH_TIMEOUT,
    SPI_FLASH_SECTOR_INVALID,
    SPI_FLASH_BLOCK_INVALID,
} spi_flash_err_t;


typedef uint32_t spi_dataflash_addr_t;
typedef uint32_t spi_dataflash_size_t;
typedef int32_t spi_dataflash_ret_t;


extern spi_dataflash_ret_t
spi_dataflash_read (spi_dataflash_t dev, spi_dataflash_addr_t addr,
                    void *buffer, spi_dataflash_size_t len);


/** Write to dataflash using a gather approach from a vector of
    descriptors.  The idea is to coalesce writes to the dataflash
    to minimise the number of erase operations.  */
extern spi_dataflash_ret_t
spi_dataflash_writev (spi_dataflash_t dev, spi_dataflash_addr_t addr,
                      iovec_t *iov, iovec_count_t iov_count);


/** Read from dataflash using a scatter approach to a vector of
    descriptors.  */
extern spi_dataflash_ret_t
spi_dataflash_readv (spi_dataflash_t dev, spi_dataflash_addr_t addr,
                     iovec_t *iov, iovec_count_t iov_count);


extern spi_dataflash_ret_t
spi_dataflash_write (spi_dataflash_t dev, spi_dataflash_addr_t addr,
                     const void *buffer, spi_dataflash_size_t len);


extern spi_dataflash_t
spi_dataflash_init (const spi_dataflash_cfg_t *cfg);


extern void
spi_dataflash_shutdown (spi_dataflash_t dev);


#ifdef __cplusplus
}
#endif    
#endif


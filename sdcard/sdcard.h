/** @file   sdcard.h
    @author Michael Hayes
    @date   1 May 2010
    @brief  Secure digital card driver. 
*/

#ifndef SDCARD_H
#define SDCARD_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "spi.h"

enum {SDCARD_BLOCK_SIZE = 512};
enum {SDCARD_PAGE_BLOCKS = 32};
enum {SDCARD_PAGE_SIZE = SDCARD_BLOCK_SIZE * SDCARD_PAGE_BLOCKS};

typedef enum
{
    SDCARD_TYPE_SDHC = 1,
    SDCARD_TYPE_SDXC,
    SDCARD_TYPE_SD,
    SDCARD_TYPE_MMC
} sdcard_type_t;


typedef uint32_t sdcard_addr_t;
typedef uint32_t sdcard_size_t;
typedef int32_t sdcard_ret_t;
typedef uint16_t sdcard_block_t;
typedef uint16_t sdcard_status_t;

typedef struct
{
    spi_cfg_t spi;
} sdcard_cfg_t;    


typedef struct
{
    spi_t spi;
    uint32_t blocks;
    uint32_t read_timeout;
    uint32_t write_timeout;
    uint32_t speed;
    uint16_t command_timeouts;
    uint16_t read_errors;
    uint16_t read_timeouts;
    uint16_t write_errors;
    uint16_t write_rejects;
    uint16_t write_timeouts;
    sdcard_status_t command_status;
    sdcard_status_t read_status;
    sdcard_status_t write_status;
    uint8_t addr_shift;
    uint8_t status;
    sdcard_type_t type;
    bool crc_enabled;
} sdcard_dev_t;


typedef sdcard_dev_t *sdcard_t;


typedef enum
{
    SDCARD_ERR_OK = 0,
    SDCARD_ERR_NO_CARD,
    SDCARD_ERR_ERROR,
    SDCARD_ERR_WRITE_PROECT,
    SDCARD_ERR_NOT_READY,
    SDCARD_ERR_PARAM
} sdcard_err_t;


sdcard_ret_t
sdcard_read (sdcard_t dev, sdcard_addr_t addr,
             void *buffer, sdcard_size_t len);


sdcard_ret_t
sdcard_write (sdcard_t dev, sdcard_addr_t addr,
              const void *buffer, sdcard_size_t len);


sdcard_t
sdcard_init (const sdcard_cfg_t *cfg);


sdcard_addr_t
sdcard_capacity_get (sdcard_t dev);


sdcard_err_t
sdcard_probe (sdcard_t dev);


void
sdcard_shutdown (sdcard_t dev);


#ifdef __cplusplus
}
#endif    
#endif


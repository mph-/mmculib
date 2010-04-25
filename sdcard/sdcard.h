/** @file sdcard.h
 *  @author Michael Hayes
 *  @date 06/08/03
 * 
 *  @brief Routines to communicate with an sdcard.
 */
 
#ifndef SDCARD_H
#define SDCARD_H

#include "config.h"
#include "spi.h"

enum {SDCARD_BLOCK_SIZE = 512};


typedef struct
{
    spi_cfg_t spi;
} sdcard_cfg_t;    


typedef struct
{
    spi_t spi;
    uint8_t status;
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


typedef uint32_t sdcard_addr_t;
typedef uint32_t sdcard_size_t;
typedef int32_t sdcard_ret_t;
typedef uint16_t sdcard_block_t;


extern sdcard_ret_t
sdcard_read (sdcard_t dev, sdcard_addr_t addr,
             void *buffer, sdcard_size_t len);


extern sdcard_ret_t
sdcard_write (sdcard_t dev, sdcard_addr_t addr,
              const void *buffer, sdcard_size_t len);


extern sdcard_t
sdcard_init (const sdcard_cfg_t *cfg);


extern sdcard_err_t
sdcard_probe (sdcard_t dev);


extern void
sdcard_shutdown (sdcard_t dev);

#endif

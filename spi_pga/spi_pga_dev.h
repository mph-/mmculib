/** @file   spi_pga_dev.h
    @author M. P. Hayes, UCECE
    @date   09 August 2007
    @brief  SPI PGA
*/

#ifndef SPI_PGA_DEV_H
#define SPI_PGA_DEV_H

#include "config.h"
#include "spi.h"

typedef struct spi_pga_dev_struct *spi_pga_t;


typedef uint16_t spi_pga_gain_t;
typedef int16_t spi_pga_offset_t;
typedef uint8_t spi_pga_channel_t;


typedef bool
(*spi_pga_gain_set_t)(spi_pga_t pga, uint8_t gain_index);


typedef spi_pga_channel_t
(*spi_pga_channel_set_t)(spi_pga_t pga, spi_pga_channel_t channel);


typedef spi_pga_offset_t
(*spi_pga_offset_set_t)(spi_pga_t pga, spi_pga_offset_t offset, bool enable);


typedef bool
(*spi_pga_shutdown_set_t)(spi_pga_t pga, bool enable);


typedef const spi_pga_gain_t *
(*spi_pga_gains_get_t)(spi_pga_t pga);


typedef struct spi_pga_ops_struct
{
    spi_pga_gain_set_t gain_set;
    spi_pga_channel_set_t channel_set;
    spi_pga_offset_set_t offset_set;
    spi_pga_shutdown_set_t shutdown_set;
    spi_pga_gains_get_t gains_get;
} spi_pga_ops_t;


struct spi_pga_dev_struct
{
    spi_t spi;
    spi_pga_ops_t *ops;
    spi_pga_gain_t gain;
    spi_pga_channel_t channel;
    spi_pga_offset_t offset;
};

typedef struct spi_pga_dev_struct spi_pga_dev_t;


bool
spi_pga_command (spi_pga_t pga, uint8_t *commands, uint8_t len);

#endif

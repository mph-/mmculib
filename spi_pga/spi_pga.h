/** @file   spi_pga.h
    @author M. P. Hayes, UCECE
    @date   09 August 2007
    @brief  SPI PGA
*/

#ifndef SPI_PGA_H
#define SPI_PGA_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "spi_pga_dev.h"

typedef enum
{
    SPI_PGA_DUMMY,
    SPI_PGA_MAX9939,
    SPI_PGA_MCP6S21,
    SPI_PGA_MCP6S2X
} spi_pga_type_t;


typedef struct spi_pga_cfg_struct
{
    const spi_cfg_t spi;
    spi_pga_type_t type;
} spi_pga_cfg_t;


spi_pga_gain_t 
spi_pga_gain_set (spi_pga_t pga, spi_pga_gain_t gain);


spi_pga_gain_t 
spi_pga_gain_get (spi_pga_t pga);


spi_pga_gain_t 
spi_pga_gain_next_get (spi_pga_t pga);


spi_pga_channel_t
spi_pga_channel_set (spi_pga_t pga, spi_pga_channel_t channel);


spi_pga_offset_t
spi_pga_offset_set (spi_pga_t pga, spi_pga_offset_t offset, bool measure);


bool
spi_pga_input_short_set (spi_pga_t pga, bool enable);


bool
spi_pga_shutdown (spi_pga_t pga);


bool
spi_pga_wakeup (spi_pga_t pga);


spi_pga_t
spi_pga_init (const spi_pga_cfg_t *cfg);


#ifdef __cplusplus
}
#endif    
#endif


/** @file  pga.h
    @author Tony Culliford
    @date   8 February 2005

    @brief Interface routines for Microchip PGAs (MCP6S2X)
*/


#ifndef PGA_H
#define PGA_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "spi.h"
#include "config.h"
#include "port.h"

#ifndef PGA_SPI_DIVISOR
#define PGA_SPI_DIVISOR 8
#endif


/* Define the channel register settings.  It is up to the user to
   only select channels their device is capable of handling.  */
typedef enum {PGA_CHANNEL_0 = 0,
              PGA_CHANNEL_1 = 1,
              PGA_CHANNEL_2 = 2,
              PGA_CHANNEL_3 = 3,
              PGA_CHANNEL_4 = 4,
              PGA_CHANNEL_5 = 5,
              PGA_CHANNEL_6 = 6,
              PGA_CHANNEL_7 = 7
} pga_channel_t;


/* Gain register settings.  */
typedef enum {PGA_GAIN_1 = 0, 
              PGA_GAIN_2 = 1, 
              PGA_GAIN_4 = 2, 
              PGA_GAIN_5 = 3, 
              PGA_GAIN_8 = 4,
              PGA_GAIN_10 = 5,
              PGA_GAIN_16 = 6,
              PGA_GAIN_32 = 7
} pga_gain_t;


#define PGA_GAINS {1, 2, 4, 5, 8, 10, 16, 32}

#define PGA_GAIN_MAX 32


typedef spi_cfg_t pga_cfg_t;


/* This structure is defined here so the compiler can allocate enough
   memory for it.  However, its fields should be treated as private.  */
typedef struct
{
    spi_t spi;
} pga_private_t;


typedef pga_private_t pga_obj_t;
typedef pga_obj_t *pga_t;


void pga_gain_set (pga_t pga, pga_gain_t gain);

void pga_channel_set (pga_t pga, pga_channel_t channel);

void pga_wakeup (pga_t pga);

void pga_shutdown (pga_t pga);

void pga_chip_select (pga_t pga);

void pga_chip_deselect (pga_t pga);

pga_t pga_init (pga_obj_t *dev, const pga_cfg_t *cfg);



#ifdef __cplusplus
}
#endif    
#endif



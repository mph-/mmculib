/** @file spi_eeprom.h
 *  @author Michael Hayes
 *  @date 06/08/03
 * 
 *  @brief Routines to communicate with a SPI EEPROM.
 */
 
#ifndef SPI_EEPROM_H
#define SPI_EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "pio.h"
#include "spi.h"


typedef struct
{
    spi_cfg_t spi;
    pio_t wp;
    uint16_t size;
    uint16_t page_size;
} spi_eeprom_cfg_t;


/* This structure is defined here so the compiler can allocate enough
   memory for it.  However, its fields should be treated as private.  */
typedef struct
{
    spi_t spi;
    const spi_eeprom_cfg_t *cfg;
} spi_eeprom_private_t;


typedef spi_eeprom_private_t spi_eeprom_obj_t;
typedef spi_eeprom_obj_t *spi_eeprom_t;


typedef uint16_t spi_eeprom_addr_t;
typedef uint16_t spi_eeprom_size_t;

/* Initialise the SPI EEPROM for operation.  DEV is a pointer into
   RAM that stores the state of the SPI EEPROM.  CFG is a pointer into
   ROM to define the port the SPI EEPROM is connected to.  The
   returned handle is passed to the other spi_eeprom_xxx routines to
   denote the SPI EEPROM to operate on.  */
extern spi_eeprom_t
spi_eeprom_init (spi_eeprom_obj_t *dev, const spi_eeprom_cfg_t *cfg);


/* Read SIZE bytes from ADDR into BUFFER.  */
extern spi_eeprom_size_t
spi_eeprom_read (spi_eeprom_t dev, spi_eeprom_addr_t addr, 
                 void *buffer, spi_eeprom_size_t size);


/* Write SIZE bytes to ADDR from BUFFER.  */
extern spi_eeprom_size_t 
spi_eeprom_write (spi_eeprom_t dev, spi_eeprom_addr_t addr,
                  const void *buffer, spi_eeprom_size_t size);


/* Setup SPI EEPROM for continuous writing.  */
extern uint8_t
spi_eeprom_write_setup (spi_eeprom_t dev, spi_eeprom_addr_t addr);


/* Disable SPI EEPROM.  */
uint8_t
spi_eeprom_disable (spi_eeprom_t dev);


#ifdef __cplusplus
}
#endif    
#endif


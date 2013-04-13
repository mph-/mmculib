/** @file   i2cm.h
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C master (TWI)

    This is written on a whim and is completely untested.
*/

#ifndef I2CM_H
#define I2CM_H

#include "config.h"
#include "pio.h"


typedef uint32_t i2cm_addr_t;

/* This will need changing to support 10-bit addresses.  */
typedef uint8_t i2cm_id_t;


/** I2CM device configuration structure.  */
typedef struct
{
    /* Slave address.  */
    i2cm_id_t id;

    /* Number of bytes for register address.  */
    uint8_t addr_bytes;
} i2cm_dev_cfg_t;


/** I2CM bus configuration structure.  */
typedef struct
{
    /* SCL PIO. */
    pio_t scl;

    /* SDA PIO. */
    pio_t sda;
} i2cm_bus_cfg_t;


typedef struct i2cm_dev_struct i2cm_dev_t;


/** Define datatype for handle to I2CM functions.  */
typedef i2cm_dev_t *i2cm_t;


i2cm_t
i2cm_init (const i2cm_bus_cfg_t *bus_cfg, const i2cm_dev_cfg_t *dev_cfg);


int
i2cm_write (i2cm_t dev, i2cm_addr_t addr, void *buffer, uint8_t size);


int
i2cm_read (i2cm_t dev, i2cm_addr_t addr, void *buffer, uint8_t size);


#endif

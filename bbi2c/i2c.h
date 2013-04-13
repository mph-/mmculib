/** @file   i2c.h
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C master (TWI)

    This is written on a whim and is completely untested.
*/

#ifndef I2C_H
#define I2C_H

#include "config.h"
#include "pio.h"


typedef uint32_t i2c_addr_t;

/* This will need changing to support 10-bit addresses.  */
typedef uint8_t i2c_id_t;


/** I2C slave device configuration structure.  */
typedef struct
{
    /* Slave address.  */
    i2c_id_t id;

    /* Number of bytes for register address.  */
    uint8_t addr_bytes;
} i2c_slave_cfg_t;


/** I2C bus configuration structure.  */
typedef struct
{
    /* SCL PIO. */
    pio_t scl;

    /* SDA PIO. */
    pio_t sda;
} i2c_bus_cfg_t;


typedef struct i2c_dev_struct i2c_dev_t;


/** Define datatype for handle to I2C functions.  */
typedef i2c_dev_t *i2c_t;

#endif

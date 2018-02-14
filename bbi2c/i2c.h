/** @file   i2c.h
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C master (TWI)

    This is written on a whim and is completely untested.
*/

#ifndef I2C_H
#define I2C_H

#ifdef __cplusplus
extern "C" {
#endif
    

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


typedef enum i2c_action
{
    I2C_START = 1,
    I2C_STOP = 2,
    I2C_RESTART = 4,
    I2C_READ = 8,
    I2C_WRITE = 16
} i2c_action_t;


typedef enum i2c_ret
{
    I2C_OK = 0,
    I2C_ERROR_MATCH = -1,
    I2C_ERROR_NO_ACK = -2,
    I2C_ERROR_SCL_STUCK_LOW = -3,
    I2C_ERROR_CONFLICT = -4,
    I2C_ERROR_TIMEOUT = -5,
    I2C_ERROR_BUSY = -6,
    I2C_ERROR_SCL_STUCK_HIGH = -7,
    I2C_ERROR_PROTOCOL = -8,
    I2C_SEEN_START = -9,
    I2C_SEEN_STOP = -10,
    I2C_ERROR_SCL_STUCK_HIGH2 = -11
} i2c_ret_t;


#ifdef __cplusplus
}
#endif    
#endif


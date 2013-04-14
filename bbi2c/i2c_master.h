/** @file   i2c_master.h
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C master (TWI)

    This is written on a whim and is completely untested.
*/

#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include "i2c.h"

i2c_t
i2c_master_init (const i2c_bus_cfg_t *bus_cfg, const i2c_slave_cfg_t *slave_cfg);


i2c_ret_t
i2c_master_write (i2c_t dev, i2c_addr_t addr, void *buffer, uint8_t size);


i2c_ret_t
i2c_master_read (i2c_t dev, i2c_addr_t addr, void *buffer, uint8_t size);


#endif

/** @file   i2c_slave.h
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C slave (TWI)

    This is written on a whim and is completely untested.
*/

#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "i2c.h"

i2c_t
i2c_slave_init (const i2c_bus_cfg_t *bus_cfg, const i2c_slave_cfg_t *slave_cfg);


i2c_ret_t
i2c_slave_write (i2c_t dev, void *buffer, uint8_t size, int timeout_us);


i2c_ret_t
i2c_slave_read (i2c_t dev, void *buffer, uint8_t size, int timeout_us);



#ifdef __cplusplus
}
#endif    
#endif


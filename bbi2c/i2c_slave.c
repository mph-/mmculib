/** @file   i2c_slave.c
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C slave (TWI)

    This is written on a whim and is completely untested.

    Two PIOs are required for SCL and SDA.  Ideally, they should have
    schmitt trigger inputs to handle the slow rising edges of an
    open-drain bus.
*/

#include "i2c_slave.h"
#include "i2c_private.h"


i2c_t
i2c_slave_init (const i2c_bus_cfg_t *bus_cfg, const i2c_slave_cfg_t *slave_cfg)
{
    static i2c_dev_t device;
    static bool init = 0;
    i2c_dev_t *dev;

    if (init)
        return 0;

    init = 1;
    dev = &device;
    
    dev->bus = bus_cfg;
    dev->slave = slave_cfg;
    
    i2c_sda_set (dev, 1);
    i2c_scl_set (dev, 1);

    return dev;
}


i2c_ret_t
i2c_slave_listen (i2c_t dev, i2c_addr_t *addr)
{
    return I2C_ERROR_MATCH;
}


i2c_ret_t
i2c_slave_write (i2c_t dev, void *buffer, uint8_t size)
{
    return 0;
}


i2c_ret_t
i2c_slave_read (i2c_t dev,  void *buffer, uint8_t size)
{
    return 0;
}

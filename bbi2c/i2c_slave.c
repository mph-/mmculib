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


static i2c_ret_t
i2c_slave_send_bit (i2c_t dev, bool bit)
{
    i2c_ret_t ret;

    /* The scl line should be low at this point.  */

    ret = i2c_scl_wait (dev);
    if (ret != I2C_OK)
        return ret;

    i2c_sda_set (dev, bit);
    return 1;
}


static i2c_ret_t 
i2c_slave_send_byte (i2c_t dev, uint8_t data)
{
    int i;
    i2c_ret_t ret;

    for (i = 0; i < 8; i++)
    {
        ret = i2c_slave_send_bit (dev, (data >> 7) & 1);
        if (ret != I2C_OK)
            return ret;
        data <<= 1;
    }

    return ret;
}


static i2c_ret_t
i2c_slave_recv_bit (i2c_t dev)
{
    i2c_ret_t ret;

    /* The scl line should be low at this point.  */

    ret = i2c_scl_wait (dev);
    if (ret != I2C_OK)
        return ret;

    return i2c_sda_get (dev);
}


static i2c_ret_t
i2c_slave_recv_byte (i2c_t dev, uint8_t *data)
{
    int i;
    i2c_ret_t ret;
    uint8_t d = 0;

    for (i = 0; i < 8; i++)
    {
        ret = i2c_slave_recv_bit (dev);
        if (ret != I2C_OK)
            return ret;

        d = (d << 1) | ret;
    }

    *data = d;

    /* CHECKME.  */
    i2c_slave_send_bit (dev, 1);

    return I2C_OK;
}


static int
i2c_slave_send_data (i2c_t dev, void *buffer, uint8_t size)
{
    uint8_t i;
    uint8_t *data = buffer;

    /* Send data packets.  */
    for (i = 0; i < size; i++)
    {
        i2c_ret_t ret;

        ret = i2c_slave_send_byte (dev, data[i]);
        if (ret != I2C_OK)
            return ret;
    }
    return i;
}


static i2c_ret_t
i2c_slave_recv_data (i2c_t dev, void *buffer, uint8_t size)
{
    uint8_t i;
    uint8_t *data = buffer;

    /* Receive data packets.  */
    for (i = 0; i < size; i++)
    {
        i2c_ret_t ret;

        ret = i2c_slave_recv_byte (dev, &data[i]);
        if (ret != I2C_OK)
            return ret;
    }
    return i;
}


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
    return i2c_slave_send_data (dev, buffer, size);
}


i2c_ret_t
i2c_slave_read (i2c_t dev,  void *buffer, uint8_t size)
{
    return i2c_slave_recv_data (dev, buffer, size);
}

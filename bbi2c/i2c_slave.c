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

    i2c_sda_set (dev, bit);

    /* Wait for scl to go high.  */
    ret = i2c_scl_wait_high (dev);
    if (ret != I2C_OK)
        return ret;

    ret = i2c_scl_wait_low (dev);
    if (ret != I2C_OK)
        return ret;

    return I2C_OK;
}


static i2c_ret_t
i2c_slave_send_ack (i2c_t dev)
{
    i2c_ret_t ret;

    ret = i2c_slave_send_bit (dev, 0);
    if (ret != I2C_OK)
        return ret;

    i2c_sda_set (dev, 1);

    return I2C_OK;
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

    ret = i2c_scl_wait_low (dev);
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

    /* Don't send acknowledge here; this routine is also used for
       reading the slave address and we should only send an ack if the
       address matechs.  */

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
        if (ret < 0)
            return ret;

        ret = i2c_slave_recv_bit (dev);
        if (ret < 0)
            return ret;
        if (ret != 1)
            return I2C_ERROR_NO_ACK;
    }
    return i;
}


static i2c_ret_t
i2c_slave_recv_data (i2c_t dev, void *buffer, uint8_t size)
{
    uint8_t i;
    uint8_t *data = buffer;

    /* TODO: handle case if requesting more bytes than sent.  In this
       case a stop will be received.  */

    /* Receive data packets.  */
    for (i = 0; i < size; i++)
    {
        i2c_ret_t ret;

        ret = i2c_slave_recv_byte (dev, &data[i]);
        if (ret < 0)
            return ret;

        /* Send acknowledge.  */
        i2c_slave_send_ack (dev);
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
i2c_slave_start_wait (i2c_t dev, int timeout_us)
{
    while (timeout_us && !i2c_scl_get (dev))
    {
        DELAY_US (1);
        timeout_us--;
    }

    if (!timeout_us)
        return I2C_ERROR_TIMEOUT;

    while (timeout_us && i2c_sda_get (dev))
    {
        /* If scl goes low then we missed the start.  */
        if (!i2c_scl_get (dev))
            return I2C_ERROR_BUSY;

        DELAY_US (1);
        timeout_us--;
    }    

    if (!timeout_us)
        return I2C_ERROR_TIMEOUT;

    return I2C_OK;
}


i2c_ret_t
i2c_slave_listen (i2c_t dev, i2c_addr_t *addr, int timeout_us)
{
    i2c_ret_t ret;
    uint8_t id;

    /* Ensure scl is an input after an error.  */
    i2c_scl_set (dev, 1);

    i2c_slave_start_wait (dev, timeout_us);

    ret = i2c_slave_recv_byte (dev, &id);
    if (ret != I2C_OK)
        return ret;

    /* TODO: If id is zero then a general call has been transmitted.
       All slaves should respond.  */

    if ((id >> 1) != dev->slave->id)
        return I2C_ERROR_MATCH;        

    /* Send acknowledge.  */
    i2c_slave_send_ack (dev);

    /* Read register address.  */
    ret = i2c_slave_recv_data (dev, addr, dev->slave->addr_bytes);
    if (ret != I2C_OK)
        return ret;

    /* Start clock stretch by holding clock low; this gives some
       pondering time.  */
    i2c_scl_set (dev, 0);


    /* TODO:  need to consider if a start is next sent or a data
       byte.  */

    if (id & 1)
        return I2C_SLAVE_WRITE;
    
    return I2C_SLAVE_READ;
}


i2c_ret_t
i2c_slave_write (i2c_t dev, void *buffer, uint8_t size)
{
    /* Release clock stretch.  */
    i2c_scl_set (dev, 1);

    return i2c_slave_send_data (dev, buffer, size);
}


i2c_ret_t
i2c_slave_read (i2c_t dev,  void *buffer, uint8_t size)
{
    /* Release clock stretch.  */
    i2c_scl_set (dev, 1);

    return i2c_slave_recv_data (dev, buffer, size);
}

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
#include "pio.h"


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
    bool val;
    int timeout = I2C_CLOCK_STRETCH_TIMEOUT_US;

    /* The scl line should be low at this point.  */

    ret = i2c_scl_wait_high (dev);
    if (ret != I2C_OK)
        return ret;

    val = i2c_sda_get (dev);

    /* Wait for scl to go low or for SDA to transistion
       indicating a start or a stop bit.  */
    while (timeout && i2c_scl_get (dev))
    {
        DELAY_US (1);
        timeout--;

        if (val != i2c_sda_get (dev))
            return val ? I2C_SEEN_START : I2C_SEEN_STOP;

    }
    if (!timeout)
    {
        /* scl seems to be stuck high.  */
        return I2C_ERROR_SCL_STUCK_HIGH;
    }

    return val;
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

        if (ret < I2C_OK)
        {
            return ret;
        }

        d = (d << 1) | ret;
    }
    
    *data = d;

    /* Don't send acknowledge here; this routine is also used for
       reading the slave address and we should only send an ack if the
       address matechs.  */

    return I2C_OK;
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
    dev->seen_start = 0;

    /* Ensure PIO clock enabled for PIO reading.  */
    pio_init (dev->bus->sda);
    
    i2c_sda_set (dev, 1);
    i2c_scl_set (dev, 1);

    return dev;
}


i2c_ret_t
i2c_slave_start_wait (i2c_t dev, int timeout_us)
{
    /* Wait until SDA goes low.  */
    while (timeout_us && i2c_sda_get (dev))
    {
        DELAY_US (1);
        timeout_us--;
    }

    if (!timeout_us)
        return I2C_ERROR_TIMEOUT;

    while (timeout_us && i2c_scl_get (dev))
    {
        /* If sda goes high then we missed the start.  */
        if (!i2c_sda_get (dev))
            return I2C_ERROR_BUSY;

        DELAY_US (1);
        timeout_us--;
    }    


    if (!timeout_us)
        return I2C_ERROR_TIMEOUT;

    return I2C_OK;
}


i2c_ret_t
i2c_slave_read (i2c_t dev, void *buffer, uint8_t size, int timeout_us)
{
    i2c_ret_t ret;
    uint8_t id;
    uint8_t *data = buffer;
    uint8_t i;

    /* Ensure scl is an input after an error or if clock stretched.  */
    i2c_scl_set (dev, 1);

    if (!dev->seen_start)
    {
        ret = i2c_slave_start_wait (dev, timeout_us);
        if (ret != I2C_OK)
            return ret;
    }

        
    dev->seen_start = 0;
    

    ret = i2c_slave_recv_byte (dev, &id);
    if (ret != I2C_OK)
        return ret;
    
    /* TODO: If id is zero then a general call has been transmitted.
       All slaves should respond.  */

    
    if ((id >> 1) != dev->slave->id)
        return I2C_ERROR_MATCH;
    
    /* Send acknowledge.  */
    i2c_slave_send_ack (dev);

    /* If the LSB is set then there is a protocol error; we should be writing.  */
    if (id & 1)
        return I2C_ERROR_PROTOCOL;


    /* Receive data packets.  */
    for (i = 0; i < size; i++)
    {
        i2c_ret_t ret;
           

        ret = i2c_slave_recv_byte (dev, &data[i]);

        /* If saw a stop then cannot read as many bytes as requested.  */
        if (ret == I2C_SEEN_STOP)
            return i;

        /* Saw a repeated start.  */
        if (ret == I2C_SEEN_START)
        {
            dev->seen_start = 1;
            /* Stretch clock to give some pondering time.  */        
            i2c_scl_set (dev, 0);
            return i;
        }

        if (ret < I2C_OK)
        {
            return ret;
        }

        /* Send acknowledge.  */
        i2c_slave_send_ack (dev);
    }

    /* Not all data read, so stretch clock to give some pondering
       time.  */        
    i2c_scl_set (dev, 0);
    return i;
}


i2c_ret_t
i2c_slave_write (i2c_t dev, void *buffer, uint8_t size, int timeout_us)
{
    i2c_ret_t ret;
    uint8_t id;
    uint8_t i;
    uint8_t *data = buffer;

    /* Ensure scl is an input after an error or if clock stretched.  */
    i2c_scl_set (dev, 1);

    i2c_slave_start_wait (dev, timeout_us);

    ret = i2c_slave_recv_byte (dev, &id);
    if (ret != I2C_OK)
        return ret;

    if ((id >> 1) != dev->slave->id)
        return I2C_ERROR_MATCH;        

    /* Send acknowledge.  */
    i2c_slave_send_ack (dev);

    /* If the LSB is not set then there is a protocol error; we should
       be reading.  */
    if ((id & 1) == 0)
        return I2C_ERROR_PROTOCOL;

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

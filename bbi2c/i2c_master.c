/** @file   i2c_master.c
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C master (TWI)

    Two PIOs are required for SCL and SDA. 
*/

#include "i2c_master.h"
#include "i2c_private.h"


#ifndef I2C_DEVICES_NUM
#define I2C_DEVICES_NUM 4
#endif



static uint8_t i2c_devices_num = 0;
static i2c_dev_t i2c_devices[I2C_DEVICES_NUM];



/**
   Preconditions: SCL output low, SDA don't care.
   Postconditions: SCL output low, SDA input
*/
static i2c_ret_t
i2c_master_recv_bit (i2c_t dev)
{
    i2c_ret_t ret;
    bool bit;

    i2c_sda_set (dev, 1);
    i2c_delay (dev);    

    ret = i2c_scl_ensure_high (dev);
    if (ret != I2C_OK)
        return ret;

    bit = i2c_sda_get (dev);

    i2c_delay (dev);    

    i2c_scl_set (dev, 0);
    
    return bit;
}



static i2c_ret_t
i2c_master_send_bit (i2c_t dev, bool bit)
{
    i2c_ret_t ret;

    /* The scl line should be low at this point.  The sda line can
     only be changed when scl is low.  */

    i2c_sda_set (dev, bit);

    i2c_delay (dev);

    /* The receiver samples on the rising edge of scl but this is a
       slow transition.  Float scl high and wait until it goes high to
       ensure that the receiver sees the bit.  The slave can also
       force scl low to stretch the clock and give it time to do
       something.  */
    ret = i2c_scl_ensure_high (dev);
    if (ret != I2C_OK)
        return ret;

    /* Check if lost arbitration.  */
    if (bit && !i2c_sda_get (dev))
        return 0;

    i2c_delay (dev);
    
    i2c_scl_set (dev, 0);

    return I2C_OK;
}


static i2c_ret_t
i2c_master_recv_ack (i2c_t dev)
{
    i2c_ret_t ret;
    ret = i2c_master_recv_bit (dev);
    if (ret != 0)
        ret = I2C_ERROR_NO_ACK;
    return ret;
}


static i2c_ret_t
i2c_master_send_ack (i2c_t dev)
{
    return i2c_master_send_bit (dev, 0);
}


static i2c_ret_t
i2c_master_send_nack (i2c_t dev)
{
    return i2c_master_send_bit (dev, 1);
}


/**
   Preconditions: SCL output low, SDA output indeterminate.
   Postconditions: SCL output low, SDA input
*/
static i2c_ret_t 
i2c_master_send_byte (i2c_t dev, uint8_t data)
{
    int mask;
    i2c_ret_t ret;

    /* Send bytes MSB first.  */
    for (mask = 0x80; mask; mask >>= 1)
    {
        ret = i2c_master_send_bit (dev, (data & mask) != 0);
        if (ret != I2C_OK)
            return ret;
    }

    return i2c_master_recv_ack (dev);
}


static i2c_ret_t
i2c_master_recv_byte (i2c_t dev, uint8_t *data)
{
    int i;
    i2c_ret_t ret;
    uint8_t d = 0;

    for (i = 0; i < 8; i++)
    {
        ret = i2c_master_recv_bit (dev);
        if (ret < 0)
            return ret;

        d = (d << 1) | ret;
    }

    *data = d;

    return I2C_OK;
}


static i2c_ret_t
i2c_master_send_start (i2c_t dev)
{
    i2c_ret_t ret;

    i2c_sda_set (dev, 1);
    i2c_scl_set (dev, 1);

    /* The scl and sda lines should be high inputs at this point.  If
       not, some other master got in first.  */
    if (!i2c_sda_get (dev))
        return I2C_ERROR_CONFLICT;

    ret = i2c_scl_ensure_high (dev);
    if (ret != I2C_OK)
        return ret;
    i2c_sda_set (dev, 0);
    i2c_delay (dev);

    i2c_scl_set (dev, 0);
    return I2C_OK;
}


static i2c_ret_t
i2c_master_send_stop (i2c_t dev)
{
    i2c_ret_t ret;
    
    i2c_sda_set (dev, 0);
    i2c_delay (dev);

    ret = i2c_scl_ensure_high (dev);
    if (ret != I2C_OK)
        return ret;

    i2c_delay (dev);

    i2c_sda_set (dev, 1);
    /* It is possible to lose arbitration at this point but who cares? 
       We think we have finished!  */
    return I2C_OK;
}


static i2c_ret_t
i2c_master_send_addr (i2c_t dev, bool read)
{
    /* Send 7-bit slave address followed by a zero bit for writes or
       a one bit for reads.

       For 10-bit slave addresses, the second byte is part of the
       data packet.  */
    return i2c_master_send_byte (dev, (dev->slave_addr << 1) | (read != 0));
}


int
i2c_master_transfer (i2c_t dev, void *buffer, uint8_t size, i2c_action_t action)
{
    uint8_t i;
    uint8_t *data = buffer;
    i2c_ret_t ret;
    
    if ((action & I2C_START) || (action & I2C_RESTART))
    {
        ret = i2c_master_send_start (dev);
        if (ret != I2C_OK)
            return ret;

        ret = i2c_master_send_addr (dev, action & I2C_READ);
        if (ret != I2C_OK)
        {
            i2c_master_send_stop (dev);
            return ret;
        }
    }

    /* Send or receive data packets.  */
    for (i = 0; i < size; i++)
    {
        if (action & I2C_WRITE)
        {
            ret = i2c_master_send_byte (dev, data[i]);
        }
        else
        {
            ret = i2c_master_recv_byte (dev, &data[i]);
            /* Send ack (0) for every byte except the last one.  */
            i2c_master_send_bit (dev, i == size - 1);
        }

        if (ret != I2C_OK)
        {
            i2c_master_send_stop (dev);
            return ret;
        }
    }

    if (action & I2C_STOP)
    {
        ret = i2c_master_send_stop (dev);
        if (ret != I2C_OK)
            return ret;
    }

    return i;
}


i2c_ret_t
i2c_master_addr_read (i2c_t dev, i2c_id_t slave_addr,
                      i2c_addr_t addr, uint8_t addr_size,
                      void *buffer, uint8_t size)
{
    i2c_ret_t ret;

    dev->slave_addr = slave_addr;

    ret = i2c_master_transfer (dev, &addr, addr_size, 
                               I2C_START | I2C_WRITE);
    if (ret < 0)
        return ret;

    return i2c_master_transfer (dev, buffer, size, 
                                I2C_RESTART | I2C_READ | I2C_STOP);
}


i2c_ret_t
i2c_master_read (i2c_t i2c, i2c_id_t slave_addr,
                 void *buffer, uint8_t size)
{
    return i2c_master_addr_read (i2c, slave_addr, 0, 0, buffer, size);
}


i2c_ret_t
i2c_master_addr_write (i2c_t dev, i2c_id_t slave_addr,
                       i2c_addr_t addr, uint8_t addr_size,
                       void *buffer, uint8_t size)
{
    i2c_ret_t ret;

    dev->slave_addr = slave_addr;

    ret = i2c_master_transfer (dev, &addr, addr_size, 
                               I2C_START | I2C_WRITE);
    if (ret < 0)
        return ret;

    return i2c_master_transfer (dev, buffer, size, I2C_WRITE | I2C_STOP);
}


i2c_ret_t
i2c_master_write (i2c_t i2c, i2c_id_t slave_addr,
                  void *buffer, uint8_t size)
{
    return i2c_master_addr_write (i2c, slave_addr, 0, 0, buffer, size);
}


i2c_t
i2c_master_init (const i2c_bus_cfg_t *bus_cfg)
{
    i2c_dev_t *dev;

    if (i2c_devices_num >= I2C_DEVICES_NUM)
        return 0;

    dev = i2c_devices + i2c_devices_num;
    i2c_devices_num++;    

    dev->bus = bus_cfg;
    dev->slave_addr = 0;

    /* Ensure PIO clock enabled for PIO reading.  */
    pio_init (dev->bus->sda);
    
    i2c_sda_set (dev, 1);
    i2c_scl_set (dev, 1);

    return dev;
}

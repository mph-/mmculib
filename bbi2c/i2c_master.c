/** @file   i2c.c
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C master (TWI)

    This is written on a whim and is completely untested.

    Two PIOs are required for SCL and SDA.  Ideally, they should have
    schmitt trigger inputs to handle the slow rising edges of an
    open-drain bus.
*/

#include "i2c_master.h"
#include "delay.h"


#ifndef I2C_DEVICES_NUM
#define I2C_DEVICES_NUM 4
#endif


#ifndef I2C_CLOCK_STRETCH_TIMEOUT_US
#define I2C_CLOCK_STRETCH_TIMEOUT_US 50
#endif


struct i2c_dev_struct
{
    const i2c_bus_cfg_t *bus;
    const i2c_slave_cfg_t *slave;
};

static uint8_t i2c_devices_num = 0;
static i2c_dev_t i2c_devices[I2C_DEVICES_NUM];



static
bool i2c_sda_get (i2c_t dev)
{
    return pio_input_get (dev->bus->sda) != 0;
}


static void
i2c_sda_set (i2c_t dev, bool state)
{
    pio_config_set (dev->bus->sda, state ? PIO_PULLUP : PIO_OUTPUT_LOW);

    /* If sda has not gone high when state == 1 then there is a bus conflict
       and the other device has won.  */
}


static
bool i2c_scl_get (i2c_t dev)
{
    return pio_input_get (dev->bus->scl) != 0;
}


static void 
i2c_scl_set (i2c_t dev, bool state)
{
    pio_config_set (dev->bus->scl, state ? PIO_PULLUP : PIO_OUTPUT_LOW);    
}


static bool
i2c_scl_wait (i2c_t dev)
{
    if (!i2c_scl_get (dev))
    {
        int i = I2C_CLOCK_STRETCH_TIMEOUT_US;
        
        while (i && !i2c_scl_get (dev))
        {
            DELAY_US (1);
            i--;
        }
        if (!i)
        {
            /* scl seems to be stuck low.  */
            return 0;
        }
    }
    return 1;
}


static int
i2c_send_bit (i2c_t dev, bool bit)
{
    /* The scl line should be low at this point.  The sda line can
     only be changed when scl is low.  */

    i2c_sda_set (dev, bit);

    i2c_scl_set (dev, 1);

    /* The receiver samples on the rising edge of scl but this is
       a slow transition.  Wait until the scl goes high to ensure
       that the receiver sees the bit.  The slave can also force
       scl low to stretch the clock and give it time to do
       something.  */
    if (!i2c_scl_wait (dev))
        return 0;

    /* Check if lost arbitration.  */
    if (bit && !i2c_sda_get (dev))
        return 0;
    
    DELAY_US (4);
    
    i2c_scl_set (dev, 0);
    return 1;
}


static
int i2c_send_byte (i2c_t dev, uint8_t data)
{
    int i;
    int ret = 1;

    for (i = 0; i < 8; i++)
    {
        if (!i2c_send_bit (dev, (data >> 7) & 1))
            return 0;
        data <<= 1;
    }

    i2c_sda_set (dev, 1);
    i2c_scl_set (dev, 1);

    /* Check acknowledge bit.  */
    if (!i2c_sda_get (dev))
        ret = 0;

    i2c_scl_set (dev, 0);
    return ret;
}


static bool
i2c_recv_bit (i2c_t dev)
{
    bool bit;

    i2c_sda_set (dev, 1);
    DELAY_US (4);    

    i2c_scl_set (dev, 1);
    /* Hmmm, what about if there is an error?  */
    if (!i2c_scl_wait (dev))
        return 0;

    bit = i2c_sda_get (dev);

    DELAY_US (4);    

    i2c_scl_set (dev, 0);
    
    return bit;
}



static
int i2c_recv_byte (i2c_t dev, uint8_t *data, bool ack)
{
    int i;
    uint8_t d = 0;

    for (i = 0; i < 8; i++)
        d = (d << 1) | i2c_recv_bit (dev);

    *data = d;

    i2c_send_bit (dev, !ack);

    i2c_scl_set (dev, 1);
    i2c_scl_set (dev, 0);
    i2c_sda_set (dev, 0);
    i2c_sda_set (dev, 1);
    return 1;
}


static void
i2c_send_start (i2c_t dev)
{
    /* The scl and sda lines should be high at this point.  If not,
       some other master got in first.  */
    i2c_sda_set (dev, 0);
    DELAY_US (4);

    i2c_scl_set (dev, 0);
}


static void
i2c_send_stop (i2c_t dev)
{
    i2c_sda_set (dev, 0);
    DELAY_US (4);

    i2c_scl_set (dev, 1);
    i2c_scl_wait (dev);
    DELAY_US (4);

    i2c_sda_set (dev, 1);
    /* It is possible to lose arbitration at this point but who cares? 
       We think we have finished!  */
}


static int
i2c_send_addr (i2c_t dev, bool read)
{
    /* Send 7-bit slave address followed by bit to indicate
       read/write.  

       For 10-bit slave addresses, the second byte is part of the
       data packet.  */
    return i2c_send_byte (dev, (dev->slave->id << 1) | (read != 0));
}


static int
i2c_send_buffer (i2c_t dev, void *buffer, uint8_t size)
{
    uint8_t i;
    uint8_t *data = buffer;

    /* Send data packets.  */
    for (i = 0; i < size; i++)
    {
        if (!i2c_send_byte (dev, data[i]))
            return 0;
    }
    return i;
}


static int
i2c_recv_buffer (i2c_t dev, void *buffer, uint8_t size)
{
    uint8_t i;
    uint8_t *data = buffer;

    /* Receive data packets.  */
    for (i = 0; i < size; i++)
    {
        if (!i2c_recv_byte (dev, &data[i], 1))
            return 0;
    }
    return i;
}


static int
i2c_start (i2c_t dev, i2c_addr_t addr, bool read)
{
    i2c_send_start (dev);

    /* Send slave address (for writing).  */
    if (!i2c_send_addr (dev, 0))
        return 0;

    /* Send register address.  */
    if (!i2c_send_buffer (dev, &addr, dev->slave->addr_bytes))
        return 0;

    if (read)
    {
        i2c_send_start (dev);
        
        /* Send slave address (for reading).  */
        if (!i2c_send_addr (dev, 1))
            return 0;
    }
    return 1;
}


static int
i2c_stop (i2c_t dev)
{
    i2c_send_stop (dev);
    return 1;
}


int
i2c_master_read (i2c_t dev, i2c_addr_t addr, void *buffer, uint8_t size)
{
    int ret = 0;

    /* Check if some other master active.  */
    if (!i2c_sda_get (dev))
        return 0;

    if (i2c_start (dev, addr, 1))
        ret = i2c_recv_buffer (dev, buffer, size);

    i2c_stop (dev);

    return ret;
}


int
i2c_master_write (i2c_t dev, i2c_addr_t addr, void *buffer, uint8_t size)
{
    int ret = 0;

    /* Check if some other master active.  */
    if (!i2c_sda_get (dev))
        return 0;

    if (i2c_start (dev, addr, 0))
        ret = i2c_send_buffer (dev, buffer, size);

    i2c_stop (dev);

    return ret;
}


i2c_t
i2c_master_init (const i2c_bus_cfg_t *bus_cfg, const i2c_slave_cfg_t *slave_cfg)
{
    i2c_dev_t *dev;

    if (i2c_devices_num >= I2C_DEVICES_NUM)
        return 0;

    dev = i2c_devices + i2c_devices_num;
    i2c_devices_num++;    

    dev->bus = bus_cfg;
    dev->slave = slave_cfg;
    
    i2c_sda_set (dev, 1);
    i2c_scl_set (dev, 1);

    return dev;
}

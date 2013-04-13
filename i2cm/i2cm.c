/** @file   i2cm.c
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C master (TWI)

    This is written on a whim and is completely untested.

    Two PIOs are required for SCL and SDA.  Ideally, they should have
    schmitt trigger inputs to handle the slow rising edges of an
    open-drain bus.
*/

#include "i2cm.h"
#include "delay.h"


#ifndef I2CM_DEVICES_NUM
#define I2CM_DEVICES_NUM 4
#endif


#ifndef I2CM_CLOCK_STRETCH_TIMEOUT_US
#define I2CM_CLOCK_STRETCH_TIMEOUT_US 50
#endif


struct i2cm_dev_struct
{
    const i2cm_bus_cfg_t *bus;
    const i2cm_dev_cfg_t *dev;
};

static uint8_t i2cm_devices_num = 0;
static i2cm_dev_t i2cm_devices[I2CM_DEVICES_NUM];



static
bool i2cm_sda_get (i2cm_t dev)
{
    return pio_input_get (dev->bus->sda) != 0;
}


static void
i2cm_sda_set (i2cm_t dev, bool state)
{
    pio_config_set (dev->bus->sda, state ? PIO_PULLUP : PIO_OUTPUT_LOW);

    /* If sda has not gone high when state == 1 then there is a bus conflict
       and the other device has won.  */
}


static
bool i2cm_scl_get (i2cm_t dev)
{
    return pio_input_get (dev->bus->scl) != 0;
}


static void 
i2cm_scl_set (i2cm_t dev, bool state)
{
    pio_config_set (dev->bus->scl, state ? PIO_PULLUP : PIO_OUTPUT_LOW);    
}


static
int i2cm_send_byte (i2cm_t dev, uint8_t data)
{
    int i;
    int ret = 1;


    /* The scl line should be low at this point.  The sda line can
     only be changed when scl is low.  */

    for (i = 0; i < 8; i++)
    {
        i2cm_sda_set (dev, (data >> 7) & 1);
        data <<= 1;
        i2cm_scl_set (dev, 1);

        /* The receiver samples on the rising edge of scl but this is
           a slow transition.  Wait until the scl goes high to ensure
           that the receiver sees the bit.  The slave can also force
           scl low to stretch the clock and give it time to do
           something.  */
        if (!i2cm_scl_get (dev))
        {
            int i = I2CM_CLOCK_STRETCH_TIMEOUT_US;
            
            while (i && !i2cm_scl_get (dev))
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

        DELAY_US (4);
        
        i2cm_scl_set (dev, 0);
    }

    i2cm_sda_set (dev, 1);
    i2cm_scl_set (dev, 1);

    /* Check acknowledge bit.  */
    if (!i2cm_sda_get (dev))
        ret = 0;

    i2cm_scl_set (dev, 0);
    return ret;
}


static
int i2cm_recv_byte (i2cm_t dev, uint8_t *data, bool ack)
{
    int i;
    uint8_t d = 0;

    for (i = 0; i < 8; i++)
    {
        d <<= 1;
        i2cm_scl_set (dev, 1);
        d |= i2cm_sda_get (dev);
        i2cm_scl_set (dev, 0);
    }
    *data = d;
    if (ack)
        i2cm_sda_set (dev, 0);
    else
        i2cm_sda_set (dev, 1);

    i2cm_scl_set (dev, 1);
    i2cm_scl_set (dev, 0);
    i2cm_sda_set (dev, 0);
    i2cm_sda_set (dev, 1);
    return 1;
}


static void
i2cm_send_start (i2cm_t dev)
{
    i2cm_scl_set (dev, 1);
    i2cm_sda_set (dev, 0);
    i2cm_scl_set (dev, 0);
    DELAY_US (4);
}


static void
i2cm_send_stop (i2cm_t dev)
{
    i2cm_sda_set (dev, 0);
    i2cm_scl_set (dev, 1);
    i2cm_sda_set (dev, 1);
}


static int
i2cm_send_addr (i2cm_t dev, bool read)
{
    /* Send 7-bit slave address followed by bit to indicate
       read/write.  

       10-bit slave addresses are not yet supported; this requires
       writing two bytes.  */
    return i2cm_send_byte (dev, (dev->dev->id << 1) | (read != 0));
}


static int
i2cm_send_buffer (i2cm_t dev, void *buffer, uint8_t size)
{
    uint8_t i;
    uint8_t *data = buffer;

    /* Send data packets.  */
    for (i = 0; i < size; i++)
    {
        if (!i2cm_send_byte (dev, data[i]))
            return 0;
    }
    return i;
}


static int
i2cm_recv_buffer (i2cm_t dev, void *buffer, uint8_t size)
{
    uint8_t i;
    uint8_t *data = buffer;

    /* Receive data packets.  */
    for (i = 0; i < size; i++)
    {
        if (!i2cm_recv_byte (dev, &data[i], 1))
            return 0;
    }
    return i;
}


static int
i2cm_start (i2cm_t dev, i2cm_addr_t addr, bool read)
{
    i2cm_send_start (dev);

    /* Send slave address (for writing).  */
    if (!i2cm_send_addr (dev, 0))
        return 0;

    /* Send register address.  */
    if (!i2cm_send_buffer (dev, &addr, dev->dev->addr_bytes))
        return 0;

    if (read)
    {
        i2cm_send_start (dev);
        
        /* Send slave address (for reading).  */
        if (!i2cm_send_addr (dev, 1))
            return 0;
    }
    return 1;
}


static int
i2cm_stop (i2cm_t dev)
{
    i2cm_send_stop (dev);
    return 1;
}


int
i2cm_read (i2cm_t dev, i2cm_addr_t addr, void *buffer, uint8_t size)
{
    int ret;

    i2cm_start (dev, addr, 1);

    ret = i2cm_recv_buffer (dev, buffer, size);

    i2cm_stop (dev);

    return ret;
}


int
i2cm_write (i2cm_t dev, i2cm_addr_t addr, void *buffer, uint8_t size)
{
    int ret;

    i2cm_start (dev, addr, 0);

    ret = i2cm_send_buffer (dev, buffer, size);

    i2cm_stop (dev);

    return ret;
}


i2cm_t
i2cm_init (const i2cm_bus_cfg_t *bus_cfg, const i2cm_dev_cfg_t *dev_cfg)
{
    i2cm_dev_t *dev;

    if (i2cm_devices_num >= I2CM_DEVICES_NUM)
        return 0;

    dev = i2cm_devices + i2cm_devices_num;
    i2cm_devices_num++;    

    dev->bus = bus_cfg;
    dev->dev = dev_cfg;
    
    i2cm_sda_set (dev, 1);
    i2cm_scl_set (dev, 1);

    return dev;
}

/** @file   i2c_slave.c
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C slave (TWI)

    This implements a polled I2C slave.  Two PIOs are required for SCL
    and SDA.  Ideally, they should have schmitt trigger inputs to
    handle the slow rising edges of an open-drain bus. 

    The polling of the start bit needs to be improved.

    The implementation could be simplified using a state machine.
 */

/*
SCL	SDA	State	Next 	Output
1	1	I	I	
1	0	I	A	Start
0	1	I	E	Error
0	0	I	E	Error
1	1	E	I
1	0	E	E
0	1	E	E
0	0	E	E
1	1	A	I	Stop
1	0	A	A	
0	1	A	E	Error
0	0	A	B
1	1	B	E	Error
1	0	B	A	0
0	1	B	C
0	0	B	B
1	1	C	D	1
1	0	C	E	Error
0	1	C	C
0	0	C	B
1	1	D	D
1	0	D	A	Restart
0	1	D	C
0	0	D	E	Error

State D is similar to the idle state (I) except that a stop has not been detected.
State E is an error state due to an invalid transition.
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
i2c_slave_send_nak (i2c_t dev)
{
    i2c_ret_t ret;

    ret = i2c_slave_send_bit (dev, 1);
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
    int timeout = I2C_TIMEOUT_US;

    /* The scl line should be low at this point.  */

    ret = i2c_scl_wait_high (dev);
    if (ret != I2C_OK)
        return ret;

    val = i2c_sda_get (dev);

    /* Wait for scl to go low or for SDA to transition
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
        return I2C_ERROR_SCL_STUCK_HIGH2;
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
            return ret;

        d = (d << 1) | ret;
    }
    
    *data = d;

    /* Don't send acknowledge here; this routine is also used for
       reading the slave address and we should only send an ack if the
       address matches.  */

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
    dev->slave_addr = slave_cfg->id;
    dev->seen_restart = 0;

    /* Ensure PIO clock enabled for PIO reading.  */
    pio_init (dev->bus->sda);
    
    i2c_sda_set (dev, 1);
    i2c_scl_set (dev, 1);

    return dev;
}


i2c_ret_t
i2c_slave_start_wait (i2c_t dev, int timeout_us)
{
    /* The sda and scl should both be high otherwise we missed the start.  */
    if (!i2c_sda_get (dev) || !i2c_scl_get (dev))
        return I2C_ERROR_BUSY;

    /* While clock is high look for the data to go low.  */
    while (timeout_us && i2c_scl_get (dev))
    {
        if (!i2c_sda_get (dev))
            return i2c_scl_wait_low (dev);
        
        DELAY_US (1);
        timeout_us--;
    }

    if (!timeout_us)
        return I2C_ERROR_TIMEOUT;
    
    return I2C_ERROR_TIMEOUT;
}


/** Read a sequence of bytes from i2c master.  */
i2c_ret_t
i2c_slave_read (i2c_t dev, void *buffer, uint8_t size, int timeout_us)
{
    i2c_ret_t ret;
    uint8_t id;
    uint8_t *data = buffer;
    uint8_t i;

    /* There are four cases to deal with:

       1. When the i2c master writes a sequence of bytes it sends a
       start bit, the slave address (with read/write bit cleared), the
       register address, the data, and then a stop bit.

       2. When an i2c master writes to a device register it sends a
       start bit, the slave address (with read/write bit cleared), the
       register address, the data, and then a stop bit.  This case is
       the same as the previous case, with the device address being
       the first byte of the data sequence.

       3. When the i2c master reads a sequence of bytes it sends a
       start bit, the slave address (with read/write bit set), waits
       for the data, and then sends a stop bit.

       4. When an i2c master reads from a device register it sends a
       start bit, the slave address (with read/write bit cleared), the
       register address, a restart bit, the slave address (with
       read/write bit set), waits for the data, and then sends a stop
       bit.  This is similar to cases 1 and 4 but with a restart bit
       instead of a stop bit.

       In each case, we assume that the entire payload is read.
    */

    /* Ensure scl is an input after an error or if clock stretched.  */
    i2c_scl_set (dev, 1);

    /* Wait for start bit.  */
    ret = i2c_slave_start_wait (dev, timeout_us);
    if (ret != I2C_OK)
        return ret;

    /* Read slave address.  */
    ret = i2c_slave_recv_byte (dev, &id);
    if (ret != I2C_OK)
        return ret;
    
    /* TODO: If id is zero then a general call has been transmitted.
       All slaves should respond.  */
    
    
    /* If the slave address does not match, perhaps we should wait 
       until the stop bit is seen?  */
    if ((id >> 1) != dev->slave_addr)
        return I2C_ERROR_MATCH;
    
    /* Send acknowledge.  */
    i2c_slave_send_ack (dev);

    /* If the LSB is set then there is a protocol error; we should be writing.  */
    if (id & 1)
        return I2C_ERROR_PROTOCOL;


    /* Receive data packets.  */
    for (i = 0; ; i++)
    {
        i2c_ret_t ret;
        uint8_t byte;

        ret = i2c_slave_recv_byte (dev, &byte);

        /* If saw a stop then cannot read as many bytes as requested.  */
        if (ret == I2C_SEEN_STOP)
            return i;

        /* Saw a repeated start.  */
        if (ret == I2C_SEEN_START)
        {
            dev->seen_restart = 1;
            /* Stretch clock to give some pondering time.  */        
            i2c_scl_set (dev, 0);
            return i;
        }

        if (ret < I2C_OK)
        {
            return ret;
        }

        if (i < size)
        {
            data[i] = byte;
            i2c_slave_send_ack (dev);
        }
        else
        {
            i2c_slave_send_nak (dev);
        }
    }
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

    /* If a restart has been already seen don't look for a start bit.  */
    if (!dev->seen_restart)
    {
        ret = i2c_slave_start_wait (dev, timeout_us);
        if (ret != I2C_OK)
            return ret;
    }

    ret = i2c_slave_recv_byte (dev, &id);
    if (ret != I2C_OK)
        return ret;

    if ((id >> 1) != dev->slave_addr)
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

        /* Ensure SDA high before checking ack bit.  */
        i2c_sda_set (dev, 1);

        if (ret < 0)
            return ret;

        ret = i2c_slave_recv_bit (dev);
        if (ret < 0)
            return ret;
        
        /* The master stops sending an ack when it has received enough data.  */
        if (ret != 0)
            return i + 1;
    }
    return i;
}

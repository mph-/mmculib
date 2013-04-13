/** @file   i2cm.c
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C master (TWI)

    This is written on a whim and is completely untested.
*/

#include "i2cm.h"
#include "delay.h"


static
bool i2cm_sda_get (i2cm_t dev)
{
    return pio_input_get (dev->sda) != 0;
}


static void
i2cm_sda_set (i2cm_t dev, bool state)
{
    pio_config_set (dev->sda, state ? PIO_PULLUP : PIO_OUTPUT_LOW);

    /* TODO: generalise for different speeds.  */
    DELAY_US (2);
}


static void 
i2cm_scl_set (i2cm_t dev, bool state)
{
    pio_config_set (dev->scl, state ? PIO_PULLUP : PIO_OUTPUT_LOW);    

    /* TODO: generalise for different speeds.  */
}


static
int i2cm_send_byte (i2cm_t dev, uint8_t data)
{
    int i;
    int ret = 1;

    for (i = 0; i < 8; i++)
    {
        i2cm_sda_set (dev, (data >> 7) & 1);
        data <<= 1;
        i2cm_scl_set (dev, 1);
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
}


static void
i2cm_send_stop (i2cm_t dev)
{
    i2cm_sda_set (dev, 0);
    i2cm_scl_set (dev, 1);
    i2cm_sda_set (dev, 1);
}


static int
i2cm_send_addr (i2cm_t dev, i2cm_id_t id, bool read)
{
    /* Send 7-bit slave address followed by bit to indicate
       read/write.  

       10-bit slave addresses are not yet supported; this requires
       writing two bytes.  */
    return i2cm_send_byte (dev, (id << 1) | (read != 0));
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
    if (!i2cm_send_addr (dev, dev->id, 0))
        return 0;

    /* Send register address.  */
    if (!i2cm_send_buffer (dev, &addr, dev->addr_bytes))
        return 0;

    if (read)
    {
        i2cm_send_start (dev);
        
        /* Send slave address (for reading).  */
        if (!i2cm_send_addr (dev, dev->id, 1))
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
i2cm_init (const i2cm_cfg_t *cfg)
{
    /* No local state is kept so use configuration pointer as handle.  */
    i2cm_dev_t *dev = cfg;
    
    i2cm_sda_set (dev, 1);
    i2cm_scl_set (dev, 1);

    return dev;
}

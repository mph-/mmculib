/** @file   i2c_private.h
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C (TWI)

    This is written on a whim and is completely untested.
*/

#ifndef I2C_PRIVATE_H
#define I2C_PRIVATE_H

#include "config.h"
#include "pio.h"
#include "delay.h"

#ifndef I2C_CLOCK_STRETCH_TIMEOUT_US
#define I2C_CLOCK_STRETCH_TIMEOUT_US 50
#endif


struct i2c_dev_struct
{
    const i2c_bus_cfg_t *bus;
    const i2c_slave_cfg_t *slave;
};


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


static i2c_ret_t
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
            return I2C_ERROR_SCL_STUCK_LOW;
        }
    }
    return I2C_OK;
}

#endif

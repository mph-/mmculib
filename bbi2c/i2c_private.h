/** @file   i2c_private.h
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C (TWI)

    This is written on a whim and is completely untested.
*/

#ifndef I2C_PRIVATE_H
#define I2C_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "pio.h"
#include "delay.h"
#include "i2c.h"

#ifndef I2C_CLOCK_STRETCH_TIMEOUT_US
#define I2C_CLOCK_STRETCH_TIMEOUT_US 50
#endif


#ifndef I2C_TIMEOUT_US
#define I2C_TIMEOUT_US 5000
#endif


#ifndef I2C_DELAY_US
#define I2C_DELAY_US 4
#endif


struct i2c_dev_struct
{
    const i2c_bus_cfg_t *bus;
    i2c_id_t slave_addr;
    bool seen_restart;
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
i2c_scl_wait_high (i2c_t dev)
{
    int timeout = I2C_CLOCK_STRETCH_TIMEOUT_US;
        
    while (timeout && !i2c_scl_get (dev))
    {
        DELAY_US (1);
        timeout--;
    }
    if (!timeout)
    {
        /* scl seems to be stuck low.  */
        return I2C_ERROR_SCL_STUCK_LOW;
    }
    return I2C_OK;
}


static i2c_ret_t
i2c_scl_wait_low (i2c_t dev)
{
    int timeout = I2C_TIMEOUT_US;
        
    while (timeout && i2c_scl_get (dev))
    {
        DELAY_US (1);
        timeout--;
    }
    if (!timeout)
    {
        /* scl seems to be stuck high.  */
        return I2C_ERROR_SCL_STUCK_HIGH;
    }
    return I2C_OK;
}


static i2c_ret_t
i2c_scl_ensure_high (i2c_t dev)
{
    i2c_scl_set (dev, 1);

    return i2c_scl_wait_high (dev);
}


static void
i2c_delay (__unused__ i2c_t dev)
{
    /* This could be generalised for different speed buses.  */
    DELAY_US (I2C_DELAY_US);
}



#ifdef __cplusplus
}
#endif    
#endif


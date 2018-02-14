/** @file   i2c_master.h
    @author M. P. Hayes, UCECE
    @date   12 April 2013
    @brief  Bit-bashed I2C master (TWI)

    This is written on a whim and is completely untested.
*/

#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "i2c.h"

i2c_t
i2c_master_init (const i2c_bus_cfg_t *bus_cfg);


int
i2c_master_transfer (i2c_t dev, void *buffer, uint8_t size,
                     i2c_action_t action);


i2c_ret_t
i2c_master_addr_write (i2c_t dev, i2c_id_t slave_addr, i2c_addr_t addr,
                       uint8_t addr_size, void *buffer, uint8_t size);


/** Write data from slave.  No internal address is sent.  */
i2c_ret_t
i2c_master_write (i2c_t i2c, i2c_id_t slave_addr,
                  void *buffer, uint8_t size);


i2c_ret_t
i2c_master_addr_read (i2c_t dev, i2c_id_t slave_addr, 
                      i2c_addr_t addr, uint8_t addr_size,
                      void *buffer, uint8_t size);

/** Read data from slave.  No internal address is sent.  */
i2c_ret_t
i2c_master_read (i2c_t i2c, i2c_id_t slave_addr,
                 void *buffer, uint8_t size);


#ifdef __cplusplus
}
#endif    
#endif


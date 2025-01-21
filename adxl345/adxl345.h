/** @file   adxl345.h
    @author M. P. Hayes
    @date   3 December 2022
    @brief  Interface routines for the ADXL345 accelerometer chip
*/

#ifndef ADXL345_H
#define ADXL345_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "config.h"
#include "twi.h"

typedef struct
{
    twi_t twi;             /* TWI bus */
    twi_slave_addr_t addr; /* Address of the ADXL345: 0x1d or 0x53  */
} adxl345_t;

/**
 * Initialise the accelerometer, attached on a TWI/I2C bus
 *
 * @param twi TWI bus on which the device exists
 * @parma slave_addre TWI address of the device
 * @return adxl345_t object, or NULL on failure
 */
adxl345_t *adxl345_init (twi_t twi, twi_slave_addr_t slave_addr);

/**
 * Return true if data ready.
 *
 * @param accel adxl345_t object pointer
 * @return true if the adxl345_t has IMU data ready, false otherwise
 */
bool adxl345_is_ready (adxl345_t *dev);

/**
 * Return raw accelerometer data.
 *
 * @param accel adxl345_t object pointer
 * @param acceldata Array of 3 16-bit integers to store the accelerometer
 * @return true if the data was read successfully, false otherwise
 */
bool adxl345_accel_read (adxl345_t *dev, int16_t acceldata[3]);


#ifdef __cplusplus
}
#endif

#endif

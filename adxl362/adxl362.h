/** @file   adxl362.h
    @author M. P. Hayes
    @date   15 December 2024
    @brief  Interface routines for the ADXL362 accelerometer chip
*/

#ifndef ADXL362_H
#define ADXL362_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "config.h"
#include "spi.h"

typedef struct
{
    spi_t spi;
} adxl362_t;


typedef struct
{
    spi_cfg_t spi;
} adxl362_cfg_t;


typedef enum
{
    ADXL362_INT1,
    ADXL362_INT2
} adxl362_intpin_t;


/**
 * Initialise the accelerometer, attached on a SPI/I2C bus
 *
 * @param spi SPI bus on which the device exists
 * @parma slave_addre SPI address of the device
 * @return adxl362_t object, or NULL on failure
 */
adxl362_t *adxl362_init (adxl362_cfg_t *cfg);

/**
 * Return true if data ready.
 *
 * @param accel adxl362_t object pointer
 * @return true if the adxl362_t has IMU data ready, false otherwise
 */
bool adxl362_is_ready (adxl362_t *dev);


void adxl362_sleep (adxl362_t *dev, bool relative, adxl362_intpin_t intpin,
                    bool active_high);


/**
 * Return raw accelerometer data.
 *
 * @param accel adxl362_t object pointer
 * @param acceldata Array of 3 16-bit integers to store the accelerometer
 * @return true if the data was read successfully, false otherwise
 */
bool adxl362_accel_read (adxl362_t *dev, int16_t acceldata[3]);


void adxl362_activity_set (adxl362_t *dev, uint16_t threshold,
                           uint16_t time);

void adxl362_inactivity_set (adxl362_t *dev, uint16_t threshold,
                             uint16_t time);

#ifdef __cplusplus
}
#endif

#endif

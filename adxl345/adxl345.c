#include "delay.h"
#include "twi.h"
#include "adxl345.h"

/* Max 400 kHz.  */
#ifndef TWI_CLOCK_SPEED_KHZ
#define TWI_CLOCK_SPEED_KHZ 100
#endif


#define ADXL345_DEVID     0x00
#define ADXL345_OFSX      0x1E
#define ADXL345_OFSY      0x1F
#define ADXL345_OFSZ      0x20
#define ADXL345_POWER_CTL 0x2D
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_DATAX0    0x32   // LSB
#define ADXL345_DATAX1    0x33   // MSB
#define ADXL345_DATAY0    0x34
#define ADXL345_DATAY1    0x35
#define ADXL345_DATAZ0    0x36
#define ADXL345_DATAZ1    0x37
#define ADXL345_FIFO_CTL  0x38
#define ADXL345_FIFO_STATUS 0x39


// We only support a single device, so we will use a single
// static allocation to store it.
static adxl345_t _static_adxl345 = {};


static uint8_t adxl345_read (adxl345_t *dev, const uint8_t addr)
{
    uint8_t response = 0;
    twi_ret_t status;

    status = twi_master_addr_read (dev->twi, dev->addr, addr, 1, &response, 1);
    if (status != 1)
        return 0x0;

    return response;
}

static bool adxl345_write (adxl345_t *dev, const uint8_t addr,
                           const uint8_t value)
{
    twi_ret_t status;

    status = twi_master_addr_write (dev->twi, dev->addr, addr, 1, &value, 1);
    return (status == 1);
}


bool adxl345_is_ready (adxl345_t *dev)
{
    uint8_t response;

    response = adxl345_read (dev, ADXL345_FIFO_STATUS);

    return (response & 0x7f) != 0;
}


static bool adxl345_read_data (adxl345_t *dev, const uint8_t addr, int16_t data[3])
{
    uint8_t rawdata[6];
    twi_ret_t status;

    status = twi_master_addr_read (dev->twi, dev->addr, addr, 1,
                                   rawdata, sizeof (rawdata));
    if (status != 6)
    {
        data[0] = data[1] = data[2] = 0;
        return false;
    }

    data[0] = ((int16_t)rawdata[1] << 8) | rawdata[0];
    data[1] = ((int16_t)rawdata[3] << 8) | rawdata[2];
    data[2] = ((int16_t)rawdata[5] << 8) | rawdata[4];

    return true;
}


bool adxl345_accel_read (adxl345_t *dev, int16_t acceldata[3])
{
    return adxl345_read_data (dev, ADXL345_DATAX0, acceldata);
}


adxl345_t *adxl345_init (twi_t twi, twi_slave_addr_t addr)
{
    uint8_t response;

    adxl345_t *dev = &_static_adxl345;
    dev->addr = addr;
    dev->twi = twi;

    /* Check the device ID.  */
    response = adxl345_read (dev, ADXL345_DEVID);

    if (response != 0xE5)
        return 0;

    /* Set stream mode.  */
    adxl345_write (dev, ADXL345_FIFO_CTL, 0x80);

    /* Enable measurements.  */
    adxl345_write (dev, ADXL345_POWER_CTL, 0x08);

    /* Set full range with 4 mg/LSB.  */
    adxl345_write (dev, ADXL345_DATA_FORMAT, 0x08);

    return dev;
}

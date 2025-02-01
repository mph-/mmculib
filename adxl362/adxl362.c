#include "delay.h"
#include "spi.h"
#include "adxl362.h"

/* Max 400 kHz.  */
#ifndef SPI_CLOCK_SPEED_KHZ
#define SPI_CLOCK_SPEED_KHZ 100
#endif


#define ADXL362_DEVID_AD     0x00
#define ADXL362_DEVID_MST    0x01
#define ADXL362_DEVID_PARTID 0x02
#define ADXL362_DEVID_REVID  0x03
#define ADXL362_XDATA      0x08   // 8 MSB
#define ADXL362_YDATA      0x09   // 8 MSB
#define ADXL362_ZDATA      0x0A   // 8 MSB
#define ADXL362_XDATA_L    0x0E   // LSB
#define ADXL362_XDATA_H    0x0F   // MSB
#define ADXL362_YDATA_L    0x10
#define ADXL362_YDATA_H    0x11
#define ADXL362_ZDATA_L    0x12
#define ADXL362_ZDATA_H    0x13
#define ADXL362_SOFT_RESET 0x1F
#define ADXL362_FIFO_CTL   0x28
#define ADXL362_FIFO_SAMPLES 0x29
#define ADXL362_ACT_INACT_CTL 0x27
#define ADXL362_INTMAP1   0x2A
#define ADXL362_INTMAP2   0x2B
#define ADXL362_POWER_CTL 0x2D
#define ADXL362_SELF_TEST 0x2E

// We only support a single device, so we will use a single
// static allocation to store it.
static adxl362_t _static_adxl362 = {};


static uint8_t adxl362_read_register (adxl362_t *dev, const uint8_t addr)
{
    uint8_t response = 0xFF;
    spi_ret_t status;
    uint8_t message[] = {0x0B, addr};

    status = spi_write (dev->spi, &message, sizeof (message), 0);
    if (status != sizeof (message))
        return 0x0;

    status = spi_read (dev->spi, &response, sizeof (response), 1);
    if (status != sizeof (response))
        return 0x0;

    return response;
}


static bool adxl362_write_register (adxl362_t *dev, const uint8_t addr,
                                    const uint8_t value)
{
    spi_ret_t status;
    uint8_t message[] = {0x0A, addr, value};

    status = spi_write (dev->spi, &message, sizeof (message), 1);
    return status == sizeof (message);
}


bool adxl362_is_ready (adxl362_t *dev)
{
    uint8_t response;

    response = adxl362_read_register (dev, ADXL362_FIFO_SAMPLES);

    return (response & 0x7f) != 0;
}


static bool adxl362_read_data (adxl362_t *dev, const uint8_t addr,
                               int16_t data[3])
{
    uint8_t rawdata[6];
    spi_ret_t status;

    // FIXME

    status = spi_read (dev->spi, rawdata, sizeof (rawdata), 1);
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


bool adxl362_accel_read (adxl362_t *dev, int16_t acceldata[3])
{
    return adxl362_read_data (dev, ADXL362_XDATA_L, acceldata);
}


void adxl362_autosleep (adxl362_t *dev)
{
    uint8_t value;

    adxl362_write_register (dev, ADXL362_INTMAP1, 0x40);
    adxl362_write_register (dev, ADXL362_ACT_INACT_CTL, 0x3F);

    // Enable autosleep
    value = adxl362_read_register (dev, ADXL362_POWER_CTL);
    value |= BIT (2);
    adxl362_write_register (dev, ADXL362_POWER_CTL, value);

    // Enable measurement bit
    value = adxl362_read_register (dev, ADXL362_POWER_CTL);
    value |= BIT (1);
    adxl362_write_register (dev, ADXL362_POWER_CTL, value);

    // Make INT1 active low on awake
    adxl362_write_register (dev, ADXL362_INTMAP1, BIT(6) | BIT (7));
    // Make INT2 active low on awake
    adxl362_write_register (dev, ADXL362_INTMAP2, BIT(6) | BIT (7));
}


adxl362_t *adxl362_init (adxl362_cfg_t *cfg)
{
    uint8_t response;
    adxl362_t *dev = &_static_adxl362;

    dev->spi = spi_init (&cfg->spi);

    /* Check the device ID.  */
    response = adxl362_read_register (dev, ADXL362_DEVID_AD);
    if (response != 0xAD)
        return 0;

    response = adxl362_read_register (dev, ADXL362_DEVID_MST);
    if (response != 0x1D)
        return 0;

    response = adxl362_read_register (dev, ADXL362_DEVID_PARTID);
    if (response != 0xF2)
        return 0;

    // CHECKME

    /* Set stream mode.  */
    adxl362_write_register (dev, ADXL362_FIFO_CTL, 0x80);

    /* Enable measurements.  */
    adxl362_write_register (dev, ADXL362_POWER_CTL, 0x08);

    return dev;
}

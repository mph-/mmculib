#include "bits.h"
#include "ltc2943.h"

#define LTC2943_SLAVE_ADDR 0x64

#define LTC2943_STATUS 0
#define LTC2943_CONTROL 1
#define LTC2943_CHARGE_MSB 2
#define LTC2943_CHARGE_LSB 3
#define LTC2943_CHARGE_THRESHOLD_HIGH_MSB 4
#define LTC2943_CHARGE_THRESHOLD_HIGH_LSB 5
#define LTC2943_CHARGE_THRESHOLD_LOW_MSB 6
#define LTC2943_CHARGE_THRESHOLD_LOW_LSB 7
#define LTC2943_VOLTAGE_MSB 8
#define LTC2943_VOLTAGE_LSB 9
#define LTC2943_VOLTAGE_THRESHOLD_HIGH_MSB 10
#define LTC2943_VOLTAGE_THRESHOLD_HIGH_LSB 11
#define LTC2943_VOLTAGE_THRESHOLD_LOW_MSB 12
#define LTC2943_VOLTAGE_THRESHOLD_LOW_LSB 13
#define LTC2943_CURRENT_MSB 14
#define LTC2943_CURRENT_LSB 15
#define LTC2943_CURRENT_THRESHOLD_HIGH_MSB 16
#define LTC2943_CURRENT_THRESHOLD_HIGH_LSB 17
#define LTC2943_CURRENT_THRESHOLD_LOW_MSB 18
#define LTC2943_CURRENT_THRESHOLD_LOW_LSB 19
#define LTC2943_TEMP_MSB 20
#define LTC2943_TEMP_LSB 21
#define LTC2943_TEMP_THRESHOLD_HIGH 22
#define LTC2943_TEMP_THRESHOLD_LOW 23


uint8_t ltc2943_read_byte (ltc2943_t *dev, const uint8_t addr)
{
    uint8_t response = 0;
    uint8_t status;
    
    status = twi_master_addr_read (dev->twi, LTC2943_SLAVE_ADDR,
                                   addr, 1, &response, 1);
    return response;
}

uint8_t ltc2943_write_byte (ltc2943_t *dev, const uint8_t addr,
                            const uint8_t value)
{
    return twi_master_addr_write (dev->twi, LTC2943_SLAVE_ADDR,
                                  addr, 1, &value, 1);
}    


uint16_t ltc2943_read_word (ltc2943_t *dev, const uint8_t addr)
{
    uint8_t msb;
    uint8_t lsb;
    
    msb = ltc2943_read_byte (dev, addr);
    lsb = ltc2943_read_byte (dev, addr + 1);
    return msb << 8 | lsb;
}


uint16_t ltc2943_write_word (ltc2943_t *dev, const uint8_t addr, uint16_t value)
{
    ltc2943_write_byte (dev, addr, value >> 8);
    return ltc2943_write_byte (dev, addr + 1, value &0xFF);
}


ltc2943_t *ltc2943_init (ltc2943_cfg_t *cfg)
{
    static ltc2943_dev_t device;
    ltc2943_dev_t *dev = &device;
    twi_cfg_t twi_cfg =
        {
            .channel = cfg->twi_channel,
            .period = TWI_PERIOD_DIVISOR (cfg->twi_clock_speed_kHz * 1000)
        };
    
    dev->twi = twi_init (&twi_cfg);
    if (! dev->twi)
        return NULL;
    
    // Set scan mode for ADC conversions and prescale of 4096
    ltc2943_write_byte (dev, LTC2943_CONTROL, 0xB8);
    if (ltc2943_read_byte (dev, LTC2943_CONTROL) != 0xB8)
        return NULL;

    dev->rsense_ohm = cfg->rsense_ohm;
    
    return dev;
}


uint16_t ltc2943_read_charge (ltc2943_t *dev)
{
    return ltc2943_read_word (dev, LTC2943_CHARGE_MSB);
}


uint16_t ltc2943_read_voltage (ltc2943_t *dev)
{
    return ltc2943_read_word (dev, LTC2943_VOLTAGE_MSB);
}


uint16_t ltc2943_read_current (ltc2943_t *dev)
{
    return ltc2943_read_word (dev, LTC2943_CURRENT_MSB);
}    


uint16_t ltc2943_read_temperature (ltc2943_t *dev)
{
    return ltc2943_read_word (dev, LTC2943_TEMP_MSB);
}


uint16_t ltc2943_get_prescale (ltc2943_t *dev)
{
    uint8_t control;
    uint16_t prescale;

    control = ltc2943_read_byte (dev, LTC2943_CONTROL);
    prescale = 1 << (BITS_EXTRACT (control, 3, 5) * 2);
    if (prescale > 4096)
        prescale = 4096;
    return prescale;
}


void ltc2943_set_prescale (ltc2943_t *dev, uint16_t prescale)
{
    int i;
    uint8_t control;    
    uint16_t prescale1 = 1;
    
    for (i = 0; i < 7; i++)
    {
        if (prescale1 <= prescale)
            break;
        prescale1 << 2; 
    }

    control = ltc2943_read_byte (dev, LTC2943_CONTROL);
    BITS_INSERT (control, i, 3, 5);
    ltc2943_write_byte (dev, LTC2943_CONTROL, control);
}


void ltc2943_shutdown (ltc2943_t *dev)
{
    ltc2943_write_byte (dev, LTC2943_CONTROL, 0X01);
}


double
ltc2943_voltage_mV (ltc2943_t *dev)
{
    return 1000 * 23.6 * ltc2943_read_voltage (dev) / 65535;
}


double
ltc2943_current_mA (ltc2943_t *dev)
{
    return 60 / dev->rsense_ohm * (ltc2943_read_current (dev) - 32767) / 32767;
}


double
ltc2943_charge_mAh (ltc2943_t *dev)
{
    int prescale;

    prescale = ltc2943_get_prescale (dev);

    return 0.340 * (50e-3 / dev->rsense_ohm) * prescale *
        (ltc2943_read_charge (dev) - 32767) / 4096;
}


double
ltc2943_temperature_C (ltc2943_t *dev)
{
    return 510.0 * ltc2943_read_temperature (dev) / 65535 - 273;
}

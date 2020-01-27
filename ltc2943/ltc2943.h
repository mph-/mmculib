#ifndef LTC2943_H
#define LTC2943_H

#include "config.h"
#include "twi.h"

typedef struct
{
    twi_channel_t twi_channel;
    uint16_t twi_clock_speed_kHz;
    double rsense_ohm;
} ltc2943_cfg_t;


typedef struct
{
    twi_dev_t *twi;
    double rsense_ohm;
} ltc2943_dev_t;


typedef ltc2943_dev_t ltc2943_t;


ltc2943_t *ltc2943_init (ltc2943_cfg_t *cfg);

uint16_t ltc2943_read_charge (ltc2943_t *dev);

uint16_t ltc2943_read_voltage (ltc2943_t *dev);

uint16_t ltc2943_read_current (ltc2943_t *dev);

uint16_t ltc2943_read_temperature (ltc2943_t *dev);

uint16_t ltc2943_get_prescale (ltc2943_t *dev);

void ltc2943_set_prescale (ltc2943_t *dev, uint16_t prescale);

void ltc2943_shutdown (ltc2943_t *dev);

double ltc2943_voltage_mV (ltc2943_t *dev);

double ltc2943_current_mA (ltc2943_t *dev);

double ltc2943_charge_mAh (ltc2943_t *dev);

double ltc2943_temperature_C (ltc2943_t *dev);

#endif

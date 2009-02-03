/** @file   pulser.c
    @author M. P. Hayes, UCECE
    @date   31 December 2007
    @brief  HV pulser for Treetap6.
    @updated 15 Jan 2009, changed to support Treetap7.
*/

#include <config.h>
#include <port.h>
#include "pulser.h"
#include "pwm.h"
#include <delay.h>

#define PULSER_DOUBLER_FREQ 10e3
#define PULSER_HV_FREQ 100e3


#ifndef PULSER_FIRE_TIME_US
#define PULSER_FIRE_TIME_US 1
#endif


/** Initialise pulser.  */
void
pulser_init (void)
{
    port_pin_config_output (PSU_ANALOG_PORT, PSU_ANALOG_BIT);
    port_pin_set_high (PSU_ANALOG_PORT, PSU_ANALOG_BIT);
    
    /* Configure pulser fire pins.  */
    port_pin_config_output (TC_FIRE_PORT, TC_FIRE_BIT);
    port_pin_set_low (TC_FIRE_PORT, TC_FIRE_BIT);
    
    port_pin_config_output (EN_VDOUBLER_PORT, EN_VDOUBLER_BIT);
    port_pin_set_low (EN_VDOUBLER_PORT, EN_VDOUBLER_BIT); // turn on the Voltage Doubler

    port_pin_config_output (PWM_HV1_PORT, PWM_HV1_BIT);
    port_pin_config_output (PWM_HV2_PORT, PWM_HV2_BIT);

    /* Ensure that these pins are low whenever the PWM is disabled for
       these channels.  */
    port_pin_set_low (PWM_HV1_PORT, PWM_HV1_BIT);
    port_pin_set_low (PWM_HV2_PORT, PWM_HV2_BIT);

    /* Initialise PWM pins.  */
    pwm_init ();

    /* Set up PWM on the two channels, anti-phase.  */
    pwm_config (PWM_CHANNEL_1,
                PWM_PERIOD_DIVISOR (PULSER_HV_FREQ),
                PWM_DUTY_DIVISOR (PULSER_HV_FREQ, 50),
                PWM_ALIGN_LEFT, PWM_POLARITY_LOW);

    pwm_config (PWM_CHANNEL_2,
                PWM_PERIOD_DIVISOR (PULSER_HV_FREQ),
                PWM_DUTY_DIVISOR (PULSER_HV_FREQ, 50),
                PWM_ALIGN_LEFT, PWM_POLARITY_HIGH);
}


void
pulser_fire (uint8_t channel)
{
    port_pin_set_high (TC_FIRE_PORT, TC_FIRE_BIT);
    DELAY_US (PULSER_FIRE_TIME_US);
    port_pin_set_low (TC_FIRE_PORT, TC_FIRE_BIT);
}


void
pulser_on (uint8_t channel)
{
    switch (channel)
    {
        case PULSER_CHANNEL_1:
            port_pin_set_high (TC_FIRE_PORT, TC_FIRE_BIT);
            break;
    }
}


void
pulser_off (uint8_t channel)
{
    switch (channel)
    {
        case PULSER_CHANNEL_1:
            port_pin_set_low (TC_FIRE_PORT, TC_FIRE_BIT);
            break;
    }
}


void 
pulser_hv_enable (void)
{
    pwm_enable (PWM_CHANNEL_1);
    pwm_enable (PWM_CHANNEL_2);

    /* 400kHz, left aligned, start low.  */
    pwm_config (PWM_CHANNEL_1,
                PWM_PERIOD_DIVISOR (PULSER_HV_FREQ),
                PWM_DUTY_DIVISOR (PULSER_HV_FREQ, 50),
                PWM_ALIGN_LEFT, PWM_POLARITY_LOW);

    /* 400kHz, left aligned, start high.  */
    pwm_config (PWM_CHANNEL_2,
                PWM_PERIOD_DIVISOR (PULSER_HV_FREQ),
                PWM_DUTY_DIVISOR (PULSER_HV_FREQ, 50),
                PWM_ALIGN_LEFT, PWM_POLARITY_HIGH);

    /* Enable PWM on HV channels.  */
    pwm_start (BIT (PWM_CHANNEL_1) | BIT (PWM_CHANNEL_2));
}


void 
pulser_hv_disable (void)
{
    /* Switch back to PIO outputs that have been setup to be low.  */
    pwm_disable (PWM_CHANNEL_1);
    pwm_disable (PWM_CHANNEL_2);

    /* Disable PWM on HV channels.  */
    pwm_stop (BIT (PWM_CHANNEL_1) | BIT (PWM_CHANNEL_2));
}


bool
pulser_hv1_ready_p (void)
{
    /* TODO.  Need to read ADC.  */
    return 0;
}


bool
pulser_hv2_ready_p (void)
{
    /* TODO.  Need to read ADC.  */
    return 0;
}


void
pulser_shutdown (void)
{
    /* Disable the voltage doubler.  */
    port_pin_set_high(EN_VDOUBLER_PORT , EN_VDOUBLER_BIT);
}

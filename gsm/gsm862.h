/** @file   gsm862.c
    @author M. P. Hayes, UCECE
    @date   4 August 2009
    @note   Driver for the GSM862 GSM module
*/

#ifndef GSM862_H
#define GSM862_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "busart.h"
#include "pio.h"

typedef enum 
{
	GSM862_OK = 0,
	GSM862_TIMEOUT = -1,
	GSM862_ERROR = -2,
	GSM862_MYSTERY = -3
} gsm862_ret_t;


typedef uint16_t gsm862_timeout_t;


/** Configuration structure.  */
typedef struct
{
    /* BUSART channel number.  */
    uint8_t channel;
    /* Clock divisor to use for GSM862 clock rate.  */
    uint16_t baud_divisor;
    /* GPIO port to use for power on/off.  */
    pio_t onoff;
    /* GPIO port to use for power monitor.  */
    pio_t pwrmon;
    /* GPIO port to use for reset.  */
    pio_t reset;
} gsm862_cfg_t;


/** Macro to initialise configuration structure.  */
#define GSM862_CFG(CHANNEL, BAUDRATE, ONOFF, PWRMON, RESET)           \
    {(CHANNEL), BUSART_BAUD_DIVISOR (BAUDRATE), ONOFF, PWRMON, RESET}


/** Private gsm862 structure.  */
typedef struct
{
    const gsm862_cfg_t *cfg;
    busart_t busart;
} gsm862_private_t;


typedef gsm862_private_t gsm862_dev_t;
typedef gsm862_dev_t *gsm862_t;


gsm862_ret_t 
gsm862_puts (gsm862_t gsm, const char *string);


char *
gsm862_gets (gsm862_t gsm, char *buffer, uint16_t size, uint16_t timeout_ms);


gsm862_ret_t 
gsm862_sms_send (gsm862_t gsm, const char *txt, const char *destination);


gsm862_ret_t 
gsm862_sms_read (gsm862_t gsm, uint8_t index, char *response, uint16_t size);

void
gsm862_gps_locate (gsm862_t gsm);

bool
gsm862_shutdown (gsm862_t gsm);

char
gsm862_control (gsm862_t gsm);

void
gsm862_gprs_init (gsm862_t gsm);

void
gsm862_gprs_senddata (gsm862_t gsm, char *data);

gsm862_t
gsm862_init(const gsm862_cfg_t *cfg);


#ifdef __cplusplus
}
#endif    
#endif


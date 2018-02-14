/** @file   nrf_config.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#ifndef NRF_CONFIG_H
#define NRF_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
    

/* This protocol is implemented on the nRF2401 chip.  */
#include "nrf2401.h"

/* Wireless devices are nRF modules at present.  */
typedef nrf_t rf_t;
typedef nrf_obj_t rf_obj_t;
typedef nrf_cfg_t rf_cfg_t;


/* Macros to attempt to reduce interdependence between this wireless code and
   the RF module it is implemented on.  The rf_setup() function still needs to
   be individually set up for the RF device being used.  */
#define RF_RX_MODE_SET(DEV) nrf_rf_dir_set (DEV, NRF_RX_MODE)

#define RF_TX_MODE_SET(DEV) nrf_rf_dir_set (DEV, NRF_TX_MODE)

#define RF_DEVICE_ENABLE(DEV) nrf_rf_enable (DEV)

#define RF_DEVICE_DISABLE(DEV) nrf_rf_standby (DEV)

#define RF_DEVICE_CHANNEL_SET(DEV, CHANNEL) \
    nrf_channel_set (DEV, (CHANNEL))

#define RF_DEVICE_ADDRESS_SET(DEV, ADDRESS) \
    nrf_address1_set (DEV, (ADDRESS))

#define RF_TRANSMIT(DEV, ADDRESS, DATA, SIZE) \
    nrf_transmit (DEV, ADDRESS, DATA, SIZE)

#define RF_RECEIVE(DEV, DATA, MS_TO_WAIT) \
    nrf_receive (DEV, DATA, MS_TO_WAIT)

#define RF_INIT(DEV, CFG) \
    nrf_init (DEV, CFG)

#define RF_SETUP(DEV, SIZE) \
    nrf_setup (DEV, SIZE)

#define RF_DATA_READY_P(DEV) \
    nrf_data_ready_p (DEV)


#ifdef __cplusplus
}
#endif    
#endif


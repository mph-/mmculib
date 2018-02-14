/* File:   nrf.h
   Author: Michael Hayes
   Date:   8 December 2004
   Description:  Routines for interfacing to a Nordic wireless link.
*/

#ifndef NRF_H
#define NRF_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "nrf2401.h"

typedef uint8_t nrf_size_t;

typedef enum
{
    NRF_CMD_ACK = 0x61,
    NRF_CMD_ANNOUNCE,
    NRF_CMD_ENUMERATE,
    NRF_CMD_ENUMERATE_ACK,
    NRF_CMD_DATA_START,
    NRF_CMD_CHANNEL_SET,
    NRF_CMD_TIMESTAMP_REQ,
    NRF_CMD_BROADCAST_MODE,
    NRF_CMD_BROADCAST_EXIT,
    NRF_CMD_DEVICE_ID_GET,
    NRF_CMD_DEVICE_ID_RESPONSE,
    NRF_CMD_RESEND_PACKET,
    NRF_CMD_ALL_PACKETS_RECEIVED,
    NRF_CMD_SIZE
} nrf_cmd_t;


typedef struct
{
    nrf_cmd_t cmd;
    nrf_device_id_t device_id;
    nrf_link_t tx;
    nrf_link_t rx;
} nrf_enumerate_t;


#define NRF_PROBE_NOT_FOUND 0xFD

#define NRF_RETRIES_MAX 30

#define NRF_BROADCAST_CHANNEL 0


extern void 
nrf_node_create (nrf_node_t *node, 
                 nrf_address_t tx_address, nrf_address_t rx_address,
                 nrf_channel_t tx_channel, nrf_channel_t rx_channel,
                 nrf_id_t tx_id, nrf_id_t rx_id);


extern void 
nrf_broadcast_node_create (nrf_node_t *node);


extern nrf_size_t
nrf_write (nrf_t rf, nrf_node_t *node, void *buffer, nrf_size_t len);


extern nrf_size_t
nrf_read (nrf_t rf, nrf_node_t *node, void *data, nrf_size_t len, 
          uint8_t ms_to_wait);


extern uint8_t
nrf_acknowledge_wait (nrf_t rf, nrf_node_t *node, 
                      nrf_cmd_t command, void *buffer, uint8_t len);


extern void
nrf_acknowledge (nrf_t rf, nrf_node_t *node, nrf_cmd_t command,
                 void *buffer, uint8_t len);

extern bool
nrf_announce (nrf_t nrf, nrf_node_t *node, uint8_t *device_id);



#ifdef __cplusplus
}
#endif    
#endif


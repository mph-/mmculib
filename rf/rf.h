/** @file  rf.h
    @author Tony Culliford / Michael Hayes
    @date   8 December 2004

    @brief Higher-level routines for interfacing to a wireless link.
*/

#ifndef RF_H
#define RF_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "nrf_config.h"


typedef enum {RF_BROADCAST_SLAVE_ID = 0xFE, 
              RF_MASTER_ID = 0xFF} rf_id_t;

typedef uint8_t rf_channel_t;

enum {RF_BROADCAST_CHANNEL = 0};

enum {RF_READ_WAIT_MS = 2};
enum {RF_WRITE_WAIT_MS = 4};
enum {RF_READY_WAIT_US = 100};
enum {RF_READY_WAIT2_US = 300};

typedef struct
{
    rf_id_t id;
    rf_channel_t channel;
} rf_node_t;


enum {RF_DEVICE_ID_SIZE = 8};

/* Define a probe as a 64-bit device ID.  */
typedef struct
{
    rf_node_t node;
    uint8_t device_id[RF_DEVICE_ID_SIZE];
} rf_probe_t;


typedef uint8_t rf_size_t;


typedef enum {RF_CMD_ACK = 0x61,
              RF_CMD_DATA_START,
              RF_CMD_CHANNEL_SET,
              RF_CMD_TIMESTAMP_REQ,
              RF_CMD_BROADCAST_MODE,
              RF_CMD_BROADCAST_EXIT,
              RF_CMD_DEVICE_ID_GET,
              RF_CMD_DEVICE_ID_RESPONSE,
              RF_CMD_ENUMERATE_DEVICE,
              RF_CMD_RESEND_PACKET,
              RF_CMD_ALL_PACKETS_RECEIVED,
              RF_CMD_SIZE} rf_cmd_t;


#define RF_PROBE_NOT_FOUND 0xFD

#define RF_PAYLOAD_SIZE NRF_PAYLOAD_SIZE

#define RF_RETRIES_MAX 30

/* Function prototypes.  */
rf_address_t *rf_address_calc (rf_id_t id);

rf_t rf_init (rf_obj_t *rf, rf_cfg_t *cfg);

uint8_t rf_acknowledge_wait (rf_t rf, rf_node_t *node, rf_cmd_t command,
                             time_t *timestamp);

void rf_acknowledge (rf_t rf, rf_node_t *node, rf_cmd_t command);

uint8_t rf_probes_enumerate (rf_t rf, rf_probe_t *probes, uint8_t probes_max);

rf_id_t rf_enumeration_response (rf_t rf, uint8_t *device_id, uint8_t *version);

void rf_read_enable (rf_t rf);

void rf_write_setup (rf_t rf, rf_node_t *node);

rf_size_t rf_write (rf_t rf, rf_node_t *node, uint8_t *data, rf_size_t size);

uint16_t rf_write_data (rf_t rf, rf_node_t *node, uint8_t *data,
                        uint16_t size);

void rf_read_setup (rf_t rf, rf_node_t *node);

bool rf_read_ready_p (rf_t rf);

uint16_t rf_read_data (rf_t rf, rf_node_t *node, uint8_t *data, uint16_t size);

rf_size_t rf_read (rf_t rf, uint8_t *data, rf_size_t size, uint8_t ms_to_wait);

void rf_node_make (rf_node_t *node, rf_id_t id, rf_channel_t channel);

rf_size_t rf_transmit (rf_t rf, rf_node_t *dst_node, uint8_t *data,
                       rf_size_t size);

rf_size_t 
rf_command (rf_t rf, rf_node_t *dst_node, rf_cmd_t command,
            void *data, rf_size_t data_size);

void 
rf_command_no_ack (rf_t rf, rf_node_t *dst_node, rf_cmd_t command,
            void *data, rf_size_t data_size);

/* The rf chip should automatically switches into standby after
 * transmission, but the function is included so that the rf chip
 * can be forced to standby when the micro is going to sleep.  */
void rf_standby (rf_t rf);



#if 0

/* There are three modes: receive, transmit, standby.  */


/* Switch into receive mode on specified channel and wait 
   for packet targeted for specified address (or broadcast address).  */
rf_read_setup ();

/* Poll for received packet.  */
rf_read_ready_p ();

/* Read packet.  */
rf_read ();
    

/* Switch into standby mode.  This is not necessary since the rf chip
   automatically switches into standby after transmission.  */
rf_standby ();


/* Send packet on specified channel to specified address.  */
rf_write (). 


/* Each node is assigned an address and a channel.  A node will only
   receive a packet if it is sent on the correct channel and if
   contains its address (the destination address).

   Currently only the master can initiate a transmission.  A slave
   acknowledges the transmission by sending an acknowledge packet
   to the well known address of the master on the channel that
   the master transmitted on.

   To allow arbitrary packet transmission between nodes it will be
   necessary to add the source address to each packet.  This could
   just be the last byte of the address.

   Initially the slaves listen on the well known slave broadcast
   address on the broadcast channel.  The master enumerates them by
   sending an enumerate message; the slaves respond sending a unique
   identifier until the master acknowledges receipt of the identifier
   with the address to use.  */


#endif




#ifdef __cplusplus
}
#endif    
#endif


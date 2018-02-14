/** @file   nrf2401.h
    @author Michael Hayes
    @date   7 December 2004
    @brief  Interface routines for the Nordic nrf2401 tranceiver chip
*/

#ifndef NRF2401_H
#define NRF2401_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "spi.h"
#include "pio.h"


enum {NRF_DEVICE_ID_SIZE = 8};


typedef uint8_t nrf_device_id_t[NRF_DEVICE_ID_SIZE];


typedef enum 
{
    NRF_BROADCAST_SLAVE_ID = 0xFE, 
    NRF_MASTER_ID = 0xFF
} nrf_id_t;


typedef uint8_t nrf_channel_t;


/* Define an address to be 5 bytes (40 bits) long.  Addresses shorter
   than this are supported but provide less noise immunity.  */
typedef struct
{
    uint8_t bytes[5];
} nrf_address_t;


typedef struct
{
    nrf_id_t id;
    nrf_address_t address;    
    nrf_channel_t channel;    
} nrf_link_t;


typedef struct
{
    nrf_link_t tx;
    nrf_link_t rx;
    /* Unique identifier for enumeration.  */
//    nrf_device_id_t device_id;
} nrf_node_t;


/* Define a 15-byte structure to hold configuration data for the nrf2401.  */
typedef struct
{
    /* The first 2 bytes are used for controlling both direct and
       ShockBurst modes.  */
    unsigned int rxen:1;         /* RX or TX operation.  */
    unsigned int rx_ch_num:7;    /* Frequency channel.  */
    unsigned int rf_pwr:2;       /* NRF Output Power.  */
    unsigned int xo_f:3;         /* Crystal Frequency.  */
    unsigned int rfdr_sb:1;      /* NRF Data Rate.  */
    unsigned int cm:1;           /* Communication Mode (Direct or ShockBurst.  */
    unsigned int rx2_en:1;       /* Enable 2 channel receive mode.  */
    
    /* The next bytes are only used in ShockBurst Mode.  */
    unsigned int crc_en:1;       /* Enable on-chip CRC checking.  */
    unsigned int crc_l:1;        /* Select 8- or 16-bit CRC.  */
    unsigned int addr_w:6;       /* Select number of address bits (both channels) */
    
    nrf_address_t addr_1;
    nrf_address_t addr_2;
    uint8_t data1_w;        /* Data payload length for RX Channel 1 in bits.  */
    uint8_t data2_w;        /* Data payload length for RX Channel 2 in bits.  */
} nrf_config_bits_t;


typedef union
{
  uint8_t bytes[15];
  nrf_config_bits_t bits;
} nrf_config_t;


/* This code only allows for one receive channel to be used at present.  */
typedef struct
{
    spi_cfg_t spi;
    pio_t cs;
    pio_t ce;
    pio_t dr;
} nrf_cfg_t;


#define NRF_CFG(CHANNEL, DIVISOR, CS_PORT, CS_BIT, CE_PORT, CE_BIT, DR_PORT, DR_BIT) {{(CHANNEL), (DIVISOR), {0, 0}}, {CS_PORT, BIT (CS_BIT)}, {CE_PORT, BIT (CE_BIT)}, {DR_PORT, BIT (DR_BIT)}}



/* Define a structure containing the pin mappings of the device, and
   its configuration data.  Any program using this driver must
   allocate enough space for this structure.  */
typedef struct
{
    const nrf_cfg_t * cfg;
    nrf_config_t config;
    spi_t spi;
} nrf_obj_t;    
    

typedef nrf_obj_t *nrf_t;


/* Maximum Payload = 32 - 7 = 25 bytes.  */
#define NRF_PAYLOAD_SIZE 25

/* Definitions for some of the bit values to clarify program.  */
enum {NRF_TX_MODE = 0, NRF_RX_MODE = 1};

/* Used to select direct or ShockBurst modes.  */
enum {NRF_DIRECT = 0, NRF_SHOCKBURST = 1};

/* Select either 8 or 16-bit CRC.  */
enum {NRF_CRC_8 = 0, NRF_CRC_16 = 1};

/* Enable or disable CRC checking.  */
enum {NRF_CRC_DISABLED = 0, NRF_CRC_ENABLED = 1};

/* The used part of the address is the first byte (0xFD), the rest is
   for noise immunity. since it has many transitions.  */
#define NRF_DEFAULT_ADDRESS {0xFD, 0xCA, 0x7E, 0xA9, 0x52}

/* NRF power levels in -dBm.  */
enum {NRF_POWER_20, NRF_POWER_10, NRF_POWER_5, NRF_POWER_0};

/* Single or dual channel reception mode.  */
enum {NRF_SINGLE_CHANNEL = 0, NRF_DUAL_CHANNEL = 1};

/* Data rate of 250kbps or 1Mbps.  */
enum {NRF_DATA_250K = 0, NRF_DATA_1M = 1};

/* Select crystal frequency - 4, 8, 12, 16 or 20 MHz.  */
enum {NRF_XTAL_FREQ_4M, NRF_XTAL_FREQ_8M, NRF_XTAL_FREQ_12M, 
      NRF_XTAL_FREQ_16M, NRF_XTAL_FREQ_20M};

/* Define the time-out time to wait for acknowledgement of a packet in ms.  */
enum {NRF_TIMEOUT_ACK_MS = 10};


enum {NRF_ACK_DELAY_US = 600};


/* Function Prototypes.  */
extern nrf_t 
nrf_init (nrf_obj_t *dev, const nrf_cfg_t *cfg);


extern bool 
nrf_setup (nrf_t nrf, uint8_t payload_size);


extern uint8_t 
nrf_transmit (nrf_t nrf, nrf_node_t *node, void *buffer, uint8_t len);


extern uint8_t 
nrf_receive (nrf_t nrf, nrf_node_t *node, void *buffer, uint8_t len, 
             uint8_t ms_to_wait);


extern bool 
nrf_data_ready_p (nrf_t nrf);



#ifdef __cplusplus
}
#endif    
#endif


/** @file   nrf24.h
    @author Andre Renaud
    @date   February 2020
    @brief  Interface routines for the NRF2401 Radio chip
*/

#ifndef NRF24_H
#define NRF24_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pio.h"
#include "spi.h"

typedef enum
{
    RF24_1MBPS = 0,
    RF24_2MBPS,
    RF24_250KBPS,
} nrf24_data_rate_t;

typedef enum
{
    RF24_CRC_DISABLED = 0,
    RF24_CRC_8,
    RF24_CRC_16,
} nrf24_crc_length_t;


/** nrf24 configuration structure.  */
typedef struct
{
    uint8_t channel;
    uint64_t address;
    uint8_t payload_size;
    pio_t ce_pio;
    pio_t irq_pio;
    spi_cfg_t spi;
} nrf24_cfg_t;


typedef struct nrf24_t nrf24_t;

nrf24_t *nrf24_init (nrf24_cfg_t *cfg);

// Switch to receiver mode and listen on configured channel
// and address.
bool nrf24_listen (nrf24_t *nrf);

// Read received data (up to max of 32 bytes).  Returns number
// of bytes read or 0 if not data available.   This switches to
// receive mode if in transmit mode.
uint8_t nrf24_read (nrf24_t *nrf, void *buffer, uint8_t len);

// Switch to transmit mode and write buffer (up to max of 32
// bytes).  Returns number of bytes written.  This can block
// for about 5 ms all going well and 15 ms if there is no response.
uint8_t nrf24_write (nrf24_t *nrf, const void *buffer, uint8_t len);

// Return true if receiver has data ready.
bool nrf24_is_data_ready (nrf24_t *nrf);

// Set channel number 0--127.
// Returns false if there was a failure setting the channel
bool nrf24_set_channel (nrf24_t *nrf, uint8_t channel);

// Set address.  By default, only the 5 least significant bytes used.
void nrf24_set_address (nrf24_t *nrf, uint64_t address);

// Set the address size in bytes (3--5).
void nrf24_set_address_size (nrf24_t *nrf, uint8_t size);

// Powers down the NRF24.
// Returns false if there was a problem with the power down
bool nrf24_power_down (nrf24_t *nrf);

// Powers up the NRF24.
// This is not normally needed unless the radio has been explicitly
// powered down.
// The radio automatically powers up whenever nrf24_read or
// nrf24_write are called
// Returns false if there was a problem with the power down
bool nrf24_power_up (nrf24_t *nrf);

// Set power amplifier level (0--3).
void nrf24_set_pa_level (nrf24_t *nrf, uint8_t level);

bool nrf24_set_data_rate (nrf24_t *nrf, nrf24_data_rate_t speed);

void nrf24_set_crc_length (nrf24_t *nrf, nrf24_crc_length_t length);

// The retransmit delay is in steps of 250 us.   count is the number
// of retries.   The default count is the maximum value 15.
void nrf24_set_retries (nrf24_t *nrf, uint8_t delay, uint8_t count);

// Enable/disable auto acknowledge.  Auto acknowledge is enabled by default.
void nrf24_set_auto_ack (nrf24_t *nrf, bool state);

void nrf24_set_dynamic_payloads (nrf24_t *nrf, bool state);


#ifdef __cplusplus
}
#endif

#endif

/* The nrf24l01+ has six pipes for reading.  Each pipe matches on a
   different address and thus one receiver can communicate with six
   transmitters.   Each pipe shares the same frequency channel.

   Each pipe can have up to a five byte configurable address.  Pipe 0
   has a unique five byte address but pipes 1--5 share the four MS
   bytes.  The LS byte must be unique for all pipes.

   When a message is transmitted, it is prefixed by an address.

   To ensure that the ACK packet from the PRX is transmitted to the
   correct PTX, the PRX takes the data pipe address where it received
   the packet and uses it as the TX address when transmitting the ACK
   packet.

   On the PTX the TX_ADDR must be the same as RX_ADDR_P0.

*/


#include <string.h>

#include "nrf24.h"
#include "delay.h"

struct nrf24_t
{
    spi_t spi;
    pio_t ce_pio;
    pio_t irq_pio;
    uint8_t address_bytes[5];
    bool wide_band;
    bool p_variant;
    uint8_t payload_size;
    uint8_t ack_payload_length;
    bool dynamic_payloads_enabled;
    bool listening;
};

// We only support a single NRF24 radio, so we will use a single
// static allocation to store it.
static nrf24_t _static_nrf = {};

#define TPD_TO_STDBY 3

#define WRITE_STATUS_US 100
#define WRITE_TIMEOUT_US 10000
#define WRITE_TIMEOUTS (WRITE_TIMEOUT_US / WRITE_STATUS_US)

// Registers
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define DYNPD	    0x1C
#define FEATURE	    0x1D

// Bits
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0
#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      6
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0
#define DPL_P5	    5
#define DPL_P4	    4
#define DPL_P3	    3
#define DPL_P2	    2
#define DPL_P1	    1
#define DPL_P0	    0
#define EN_DPL	    2
#define EN_ACK_PAY  1
#define EN_DYN_ACK  0

// Commands
#define R_REGISTER          0x00
#define W_REGISTER          0x20
#define REGISTER_MASK       0x1F
#define ACTIVATE            0x50
#define R_RX_PL_WID         0x60
#define R_RX_PAYLOAD        0x61
#define W_TX_PAYLOAD        0xA0
#define W_ACK_PAYLOAD       0xA8
#define FLUSH_TX            0xE1
#define FLUSH_RX            0xE2
#define REUSE_TX_PL         0xE3
#define W_TX_PAYLOAD_NOACK  0xB0
#define NOP                 0xFF

#define RF_DR_LOW   5
#define RF_DR_HIGH  3
#define RF_PWR_LOW  1
#define RF_PWR_HIGH 2

#define PAYLOAD_SIZE_MAX 32

static uint8_t nrf24_read_registers (nrf24_t *nrf, uint8_t reg, uint8_t *buffer,
                                     uint8_t len)
{
    uint8_t status;
    uint8_t reg_address = R_REGISTER | (REGISTER_MASK & reg);
    spi_transfer_t transfers[] =
        {
            // As the register address is sent, the status is received.
            {&reg_address, &status, 1},
            {0, buffer, len}
        };

    spi_transact (nrf->spi, transfers, 2);
    return status;
}

static uint8_t nrf24_write_registers (nrf24_t *nrf, uint8_t reg,
                                      const uint8_t *buffer, uint8_t len)
{
    uint8_t status;
    uint8_t reg_address = W_REGISTER | (REGISTER_MASK & reg);
    spi_transfer_t transfers[] =
        {
            // As the register address is sent, the status is received.
            {&reg_address, &status, 1},
            {buffer, 0, len}
        };

    spi_transact (nrf->spi, transfers, 2);
    return status;
}

static uint8_t nrf24_read_register (nrf24_t *nrf, uint8_t reg)
{
    uint8_t result;

    nrf24_read_registers (nrf, reg, &result, 1);

    return result;
}

static uint8_t nrf24_write_register (nrf24_t *nrf, uint8_t reg, uint8_t value)
{
    return nrf24_write_registers (nrf, reg, &value, 1);
}

static bool nrf24_write_register_verify (nrf24_t *nrf, uint8_t reg, uint8_t value)
{
    nrf24_write_registers (nrf, reg, &value, 1);
    uint8_t r = nrf24_read_register (nrf, reg);
    return r == value;
}

static uint8_t nrf24_write_command (nrf24_t *nrf, uint8_t command)
{
    uint8_t status;

    spi_transfer (nrf->spi, &command, &status, 1, true);
    return status;
}

static void nrf24_flush_rx (nrf24_t *nrf)
{
    nrf24_write_command (nrf, FLUSH_RX);
}

static void nrf24_flush_tx (nrf24_t *nrf)
{
    nrf24_write_command (nrf, FLUSH_TX);
}

static uint8_t nrf24_get_status (nrf24_t *nrf)
{
    return nrf24_write_command (nrf, NOP);
}

static bool nrf24_set_tx_addr (nrf24_t *nrf, uint8_t *address_bytes)
{
    uint8_t resp_addr[5] = {0};
    nrf24_write_registers (nrf, TX_ADDR, address_bytes, 5);
    nrf24_read_registers (nrf, TX_ADDR, resp_addr, 5);
    return (memcmp (address_bytes, resp_addr, 5) == 0);
}

static bool nrf24_set_rx0_addr (nrf24_t *nrf, uint8_t *address_bytes)
{
    uint8_t resp_addr[5] = {0};
    nrf24_write_registers (nrf, RX_ADDR_P0, address_bytes, 5);
    nrf24_read_registers (nrf, RX_ADDR_P0, resp_addr, 5);
    return (memcmp(address_bytes, resp_addr, 5) == 0);
}

nrf24_t *nrf24_init (nrf24_cfg_t *cfg)
{
    nrf24_t *nrf = &_static_nrf;
    spi_t spi;

    spi = spi_init (&cfg->spi);
    if (! spi)
        return 0;

    nrf->spi = spi;
    nrf->ce_pio = cfg->ce_pio;
    nrf->irq_pio = cfg->irq_pio;

    pio_config_set (nrf->ce_pio, PIO_OUTPUT_LOW);
    if (nrf->irq_pio)
        pio_config_set (nrf->irq_pio, PIO_INPUT);

    nrf24_set_address (nrf, cfg->address);

    nrf->wide_band = true;
    nrf->p_variant = false;
    if (cfg->payload_size <= PAYLOAD_SIZE_MAX)
        nrf->payload_size = cfg->payload_size;
    else
        nrf->payload_size = PAYLOAD_SIZE_MAX;
    nrf->ack_payload_length = 0;
    nrf->listening = false;

    delay_ms (5);

    if (! nrf24_write_register_verify (nrf, RX_PW_P0, nrf->payload_size))
        return false;

    if (! nrf24_write_register_verify (nrf, CONFIG, BIT (EN_CRC)))
        return false;

    // Set the maximum number of retransmissions (15).
    nrf24_set_retries (nrf, 5, 15);

    nrf24_set_pa_level (nrf, 3);

    if (nrf24_set_data_rate (nrf, RF24_250KBPS))
        nrf->p_variant = true;

    // Then set the data rate to the slowest (and most reliable)
    // speed supported by all hardware.
    nrf24_set_data_rate (nrf, RF24_1MBPS);

    // Initialize CRC and request 2-byte (16bit) CRC
    nrf24_set_crc_length (nrf, RF24_CRC_16);

    nrf24_set_dynamic_payloads (nrf, false);

    // Clear current status bits
    nrf24_write_register (nrf, STATUS, BIT (RX_DR) | BIT (TX_DS) | BIT (MAX_RT));

    nrf24_set_channel (nrf, cfg->channel);

    // Flush buffers
    nrf24_flush_rx (nrf);
    nrf24_flush_tx (nrf);

    if (! nrf24_set_tx_addr (nrf, nrf->address_bytes))
        return false;
    if (! nrf24_set_rx0_addr (nrf, nrf->address_bytes))
        return false;

    nrf24_set_auto_ack (nrf, true);

    return nrf;
}

bool nrf24_is_data_ready (nrf24_t *nrf)
{
    uint8_t status;

    if (nrf->irq_pio)
    {
        // Poll IRQ pin (active low) to see if receiver has data.
        // This is less noisy than polling over SPI.
        if (pio_input_get (nrf->irq_pio))
            return false;
    }

    // Poll status register to see if receiver ready.
    status = nrf24_get_status (nrf);
    if (! (status & BIT (RX_DR)))
        return false;

    // Could determine which pipe is ready from status.
    return true;
}

uint8_t nrf24_read (nrf24_t *nrf, void *buffer, uint8_t len)
{
    uint8_t status;
    uint8_t blank_len;
    uint8_t reg_address = R_RX_PAYLOAD;
    uint8_t *data = (uint8_t *)buffer;

    if (! nrf->listening)
    {
        // Switch to receiving mode.
        nrf24_listen (nrf);
    }

    if (! nrf24_is_data_ready (nrf))
        return 0;

    if (len > nrf->payload_size)
        len = nrf->payload_size;
    blank_len = nrf->dynamic_payloads_enabled ? 0 : nrf->payload_size - len;

    spi_transfer_t transfers[] =
        {
            {&reg_address, &status, 1},
            {0, buffer, len},
            {0, 0, blank_len},
        };

    spi_transact (nrf->spi, transfers, blank_len == 0 ? 2 : 3);

    // Clear the status RX_DR bit
    nrf24_write_register (nrf, STATUS, BIT (RX_DR));

    // Handle ack payload receipt
    if (status & BIT (TX_DS))
        nrf24_write_register (nrf, STATUS, BIT (TX_DS));

    // Check for end of transmission.
    if (! nrf24_read_register (nrf, FIFO_STATUS) & BIT (RX_EMPTY))
        return 0;

    return nrf->payload_size; // TODO: ANDRE: Should be 'len'?
}


void nrf24_set_address (nrf24_t *nrf, uint64_t address)
{
    for (int i = 0; i < 5; i++)
    {
        nrf->address_bytes[i] = address & 0xff;
        address >>= 8;
    }
}

bool nrf24_set_channel (nrf24_t *nrf, uint8_t channel)
{
    if (channel > 127)
        channel = 127;

    return nrf24_write_register_verify (nrf, RF_CH, channel);
}

bool nrf24_listen (nrf24_t *nrf)
{
    // TODO: allow other pipes with different signature
    if (! nrf24_set_rx0_addr (nrf, nrf->address_bytes))
        return false;

    nrf24_write_register (nrf, EN_RXADDR,
                          nrf24_read_register (nrf, EN_RXADDR) | BIT (ERX_P0));

    if (! nrf24_write_register_verify (nrf, CONFIG,
                                       nrf24_read_register (nrf, CONFIG) | BIT (PWR_UP) | BIT (PRIM_RX)))
        return false;

    nrf24_write_register (nrf, STATUS, BIT (RX_DR) | BIT (TX_DS) | BIT (MAX_RT));

    nrf24_flush_rx (nrf);
    nrf24_flush_tx (nrf);

    pio_output_set (nrf->ce_pio, true);
    nrf->listening = true;

    // Wait for the radio to come up
    delay_ms (TPD_TO_STDBY);

    return true;
}

bool nrf24_power_down (nrf24_t *nrf)
{
    return nrf24_write_register_verify (nrf, CONFIG, nrf24_read_register (nrf, CONFIG) & ~BIT (PWR_UP));
}

bool nrf24_power_up (nrf24_t *nrf)
{
    return nrf24_write_register_verify (nrf, CONFIG, nrf24_read_register (nrf, CONFIG) | BIT (PWR_UP));
}

uint8_t nrf24_write (nrf24_t *nrf, const void *buffer, uint8_t len)
{
    uint8_t status;
    uint32_t timeouts = 0;
    uint8_t reg_address = W_TX_PAYLOAD;
    uint8_t blank_len;
    uint8_t blank_tx = 0xff;

    if (nrf->listening)
    {
        // Switch out of listening mode.
        pio_output_set (nrf->ce_pio, false);
        nrf24_flush_tx (nrf);
        nrf24_flush_tx (nrf);
        nrf->listening = false;
    }

    // There is no need to continually update the RX and TX
    // addresses but this is a minor optimisation.
    nrf24_set_tx_addr (nrf, nrf->address_bytes);
    nrf24_set_rx0_addr (nrf, nrf->address_bytes);

    // Power up transmitter and set PRIM low.
    if (! nrf24_write_register_verify (nrf, CONFIG,
                                      (nrf24_read_register (nrf, CONFIG) | BIT (PWR_UP)) & ~BIT (PRIM_RX)))
        return 0;
    delay_ms (TPD_TO_STDBY);

    if (len > nrf->payload_size)
        len = nrf->payload_size;
    blank_len = nrf->dynamic_payloads_enabled ? 0 : nrf->payload_size - len;

    spi_transfer_t transfers[] =
        {
            {&reg_address, &status, 1},
            {buffer, 0, len},
            {0, 0, blank_len},
        };

    spi_transact (nrf->spi, transfers, blank_len == 0 ? 2 : 3);

    // A high pulse (10 us min) starts transmission.
    pio_output_set (nrf->ce_pio, true);
    DELAY_US (15);
    pio_output_set (nrf->ce_pio, false);

    // If auto acknowledge enabled, the radio switches to RX mode
    // after the data packet has been transmitted.

    // Poll for the TX_DS bit (an auto acknowledge is received) or
    // the MAX_RT bit (the maximum number of retransmits has
    // occurred).
    timeouts = WRITE_TIMEOUTS;
    while (timeouts--)
    {
        uint8_t observe_tx;

        // Read status and observe_tx.  The latter tracks the
        // number of lost and retransmitted packets for debugging.
        status = nrf24_read_registers (nrf, OBSERVE_TX, &observe_tx, 1);
        if (status & (BIT (TX_DS) | BIT (MAX_RT)))
            break;

        DELAY_US (WRITE_STATUS_US);
    }

    // Read status and then clear status bits.
    status = nrf24_write_register (nrf, STATUS, BIT (RX_DR) | BIT (TX_DS) | BIT (MAX_RT));

    // Auto acknowledge can send an ack packet.
    if (status & BIT (RX_DR))
    {
        // TODO.  This is not currently supported.
        nrf->ack_payload_length = 0;
    }

    // Clear the transmit-complete bit.
    nrf24_write_register (nrf, STATUS, nrf24_read_register (nrf, STATUS) | BIT (TX_DS));

    // Flush transmitter buffer.
    nrf24_flush_tx (nrf);

    // If had no auto acknowledge, then cannot guarantee that the
    // transmitted data packet was received.
    if (! (status & BIT (TX_DS)))
        return 0;

    // Return number of bytes transmitted.
    return nrf->payload_size; // TODO: ANDRE: Should this be 'len'?
}

// Set power amplifier level.
void nrf24_set_pa_level (nrf24_t *nrf, uint8_t level)
{
    uint8_t setup = nrf24_read_register (nrf, RF_SETUP);

    setup &= ~(BIT (RF_PWR_LOW) | BIT (RF_PWR_HIGH));

    switch (level)
    {
        default:
        case 3:
            setup |= BIT (RF_PWR_LOW) | BIT (RF_PWR_HIGH);
            break;
        case 2:
            setup |= BIT (RF_PWR_HIGH);
            break;
        case 1:
            setup |= BIT (RF_PWR_LOW);
        case 0:
            break;
    }
    nrf24_write_register (nrf, RF_SETUP, setup);
}

bool nrf24_set_data_rate (nrf24_t *nrf, nrf24_data_rate_t speed)
{
    bool result = false;
    uint8_t setup = nrf24_read_register (nrf, RF_SETUP);

    // HIGH and LOW '00' is 1Mbs - our default
    nrf->wide_band = false;
    setup &= ~ (BIT (RF_DR_LOW) | BIT (RF_DR_HIGH));
    if (speed == RF24_250KBPS)
    {
        // Must set the RF_DR_LOW to 1; RF_DR_HIGH (used to be RF_DR) is already 0
        // Making it '10'.
        nrf->wide_band = false;
        setup |= BIT (RF_DR_LOW);
    }
    else
    {
        // Set 2Mbs, RF_DR (RF_DR_HIGH) is set 1
        // Making it '01'
        if (speed == RF24_2MBPS)
        {
            nrf->wide_band = true;
            setup |= BIT (RF_DR_HIGH);
        }
        else
        {
            // 1Mbs
            nrf->wide_band = false;
        }
    }
    nrf24_write_register (nrf, RF_SETUP, setup);

    // Verify our result
    if (nrf24_read_register (nrf, RF_SETUP) == setup)
    {
        result = true;
    }
    else
    {
        nrf->wide_band = false;
    }

    return result;
}

void nrf24_set_crc_length (nrf24_t *nrf, nrf24_crc_length_t length)
{
    uint8_t config;

    config = nrf24_read_register (nrf, CONFIG) & ~ (BIT (CRCO) | BIT (EN_CRC));

    switch (length)
    {
        case RF24_CRC_DISABLED:
            break;

        case RF24_CRC_8:
            config |= BIT (EN_CRC);
            break;

        case RF24_CRC_16:
            config |= BIT (EN_CRC) | BIT (CRCO);
            break;
    }
    nrf24_write_register (nrf, CONFIG, config);
}

// The retransmit delay is in steps of 250 us.  count is the number
// of retries.
void nrf24_set_retries (nrf24_t *nrf, uint8_t delay, uint8_t count)
{
    if (delay > 15)
        delay = 15;
    if (count > 15)
        count = 15;

    nrf24_write_register (nrf, SETUP_RETR,
                         (delay & 0xf) << ARD | (count & 0xf) << ARC);
}

void nrf24_set_auto_ack (nrf24_t *nrf, bool state)
{
    // This can be controlled for each pipe.
    if (state)
        nrf24_write_register (nrf, EN_AA, 0x7f);
    else
        nrf24_write_register (nrf, EN_AA, 0x00);
}

void nrf24_set_address_size (nrf24_t *nrf, uint8_t size)
{
    if (size == 0)
    {
        nrf24_write_register (nrf, SETUP_AW, size);
        return;
    }

    if (size > 5)
        size = 5;
    else if (size < 3)
        size = 3;

    nrf24_write_register (nrf, SETUP_AW, size - 2);
}

void nrf24_set_dynamic_payloads (nrf24_t *nrf, bool state)
{
    nrf24_write_register (nrf, DYNPD, state);
    nrf->dynamic_payloads_enabled = state;
}

/** @file   nrf2401.c
    @author Michael Hayes / Tony Culliford
    @date   7 December 2004

    @brief Interface routines for the Nordic nRF2401 tranceiver chip.
*/

#include "nrf2401.h"
#include "delay.h"

#define CHAR_BIT 8

/* In NZ, the maximum allowable channel number is 83.  */
enum {NRF_CHANNEL_NUMBER_MAX = 83};


/* Use 16-bit CRC (2 bytes) */
/* Packet Overhead = Address + CRC = 7 bytes.  */

/* Maximum Burst Length = 32 bytes.  */

/* Packet Length = 11 bytes = 88 bits.  */

enum {NRF_CONFIG_DELAY_US = 20};

enum {NRF_LINE_TIME_ENABLE_US = 10};

/* Definitions of the required delays in µs between various operations.  */
enum {NRF_T_SB_ACTIVE = 202};


/* Definitions of some macros for outside functions to call.  */
#define NRF_DATA_READY_P(DEV) (pio_input_get ((DEV)->cfg->dr)


/* The nRF2401 chip has a 15-byte configuration register.  */
#define NRF_CONFIGURATION_REGISTER_SIZE 15

#define NRF_CE_HIGH_SET(DEV) (pio_output_high ((DEV)->cfg->ce))
#define NRF_CE_LOW_SET(DEV) (pio_output_low ((DEV)->cfg->ce))
#define NRF_CS_HIGH_SET(DEV) (pio_output_high ((DEV)->cfg->cs))
#define NRF_CS_LOW_SET(DEV) (pio_output_low ((DEV)->cfg->cs))


/* Unfortunately, the CS is active high.  Rather than pessimising the
   SPI driver, we manually drive CS.
   
   Mode     CS    CE
   Active    0     1
   Config    1     0
   Standby   0     0
*/


/* Exit RF Mode - assumes unit is powered up.  */
void
nrf_standby (nrf_t nrf)
{
    nrf_obj_t *dev= nrf;

    NRF_CS_LOW_SET (dev);
    NRF_CE_LOW_SET (dev);
}


/* Enable RF transceiver - assumes unit is powered up
   (Only necessary for direct transmission mode).  */
void
nrf_enable (nrf_t nrf)
{
    nrf_obj_t *dev= nrf;

    NRF_CS_LOW_SET (dev);
    NRF_CE_HIGH_SET (dev);
}


nrf_t
nrf_init (nrf_obj_t *dev, const nrf_cfg_t *cfg)
{
    dev->cfg = cfg;

    /* Make chip select pin an output.  The SPI driver does not control
     this pin since it is active high.  */
    pio_config_set (cfg->cs, PIO_OUTPUT_HIGH);

    /* Make chip enable pin an output.  */
    pio_config_set (cfg->ce, PIO_OUTPUT_HIGH);
    
    /* Make data ready pin an input.  */
    pio_config_set (cfg->dr, PIO_INPUT);
    
    /* Initialise spi port.  */
    dev->spi = spi_init (&cfg->spi);

    return dev;
}


/* Configures the nRF2401 with the given number of bytes (1 - 15) of
   configuration data.  */
static void
nrf_configure (nrf_t nrf, uint8_t size)
{
    nrf_obj_t *dev = nrf;
    uint8_t i;

    /* Put the nRF2401 into configuration mode.  */
    NRF_CE_LOW_SET (dev);
    NRF_CS_HIGH_SET (dev);

    DELAY_US (NRF_LINE_TIME_ENABLE_US);

    /* Decrementing logic is to ensure data is clocked out MSB first.  */
    for (i = size; i > 0; i--)
    {
        /* Write to the SPI port.  */
        spi_putc (dev->spi, dev->config.bytes[i - 1]);
    }

    DELAY_US (NRF_CONFIG_DELAY_US);
    
    /* Put the chip into standby mode.  */
    NRF_CS_LOW_SET (dev);
    
    DELAY_US (NRF_LINE_TIME_ENABLE_US);
}


/* Set either transmit or receive mode.  */
static void
nrf_dir_set (nrf_t nrf, uint8_t mode)
{
    nrf_obj_t *dev = nrf;

    if (dev->config.bits.rxen == mode)
        return;

    /* Set the bit to the mode value, either RX_MODE, or TX_MODE.  */
    dev->config.bits.rxen = mode;

    /* Program the first byte of the configuration register only.  */
    nrf_configure (dev, 1);
}


/* Set the channel number (Max of 83 in NZ).  */
static void
nrf_channel_set (nrf_t nrf, uint8_t channel)
{
    nrf_obj_t *dev = nrf;

    if (dev->config.bits.rx_ch_num == channel)
        return;

    /* Ensure channel is not higher than 83 for use in NZ.  */
    if (channel > NRF_CHANNEL_NUMBER_MAX)
        channel = 0;

    dev->config.bits.rx_ch_num = channel;
    
    /* Only the first byte needs to be reprogrammed for a channel change.  */
    nrf_configure (dev, 1);
}


/* All other configuration altering functions are followed by a full
   reprogram.  This is inefficient if many options need to be changed,
   but this should only happen once per program, when the link is
   first set up.  */


/* Set the RF power level.  */
static void
nrf_power_set (nrf_t nrf, uint8_t rf_power)
{
    nrf_obj_t *dev = nrf;

    dev->config.bits.rf_pwr = rf_power;
}


/* Set the crystal frequency.  */
static void
nrf_xtal_freq_set (nrf_t nrf, uint8_t freq)
{
    nrf_obj_t *dev = nrf;

    dev->config.bits.xo_f = freq;
}


/* Set the RF data rate.  */
static void
nrf_data_rate_set (nrf_t nrf, uint8_t data_rate)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.rfdr_sb = data_rate;
}


/* Set the nRF into either ShockBurst or direct mode.  */
static void
nrf_comms_mode_set (nrf_t nrf, uint8_t mode)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.cm = mode;
}


/* Set the nRF to use either single or dual-channel reception.  */
static void
nrf_single_or_dual_channel_set (nrf_t nrf, uint8_t mode)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.rx2_en = mode;
}


/* Set the nRF to enable or disable CRC checking.  */
static void
nrf_crc_status_set (nrf_t nrf, uint8_t status)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.crc_en = status;
}


/* Set the nRF to use either 8- or 16-bit CRC.  */
static void
nrf_crc_length_set (nrf_t nrf, uint8_t length)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.crc_l = length;
}


/* Set number of address bits used for both channels.  */
static void
nrf_address_length_set (nrf_t nrf, uint8_t length)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.addr_w = length;
}


/* Set the nRF channel 1 RX address.  */
static void
nrf_address1_set (nrf_t nrf, nrf_address_t *address)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.addr_1 = *address;
}


/* Set the nRF channel 2 RX address.  */
static void
nrf_address2_set (nrf_t nrf, nrf_address_t *address)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.addr_2 = *address;
}


/* Set the payload length for channel 1.  */
static void
nrf_payload_length1_set (nrf_t nrf, uint8_t length)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.data1_w = length;
}


/* Set the payload length for channel 2.  */
static void
nrf_payload_length2_set (nrf_t nrf, uint8_t length)
{
    nrf_obj_t *dev= nrf;

    dev->config.bits.data2_w = length;
}


/* Function to get the payload length in bytes.  */
static uint8_t
nrf_payload_length1_get (nrf_t nrf)
{
    nrf_obj_t *dev= nrf;

    return dev->config.bits.data1_w / CHAR_BIT;
}


/* This blocks for up to ms_to_wait.  */
uint8_t
nrf_receive (nrf_t nrf, nrf_node_t *node, void *buffer, uint8_t len, 
             uint8_t ms_to_wait)
{
    nrf_obj_t *dev = nrf;
    uint8_t packet_length = 0;
    uint8_t i;

    /* Changing channel requires a one byte configure.  */
    nrf_channel_set (dev, node->rx.channel);

    /* Changing the address requires a full configure.  */
    nrf_address1_set (dev, &node->rx.address);

    /* Changing direction requires a one byte configure.  */
    nrf_dir_set (dev, NRF_RX_MODE);

    nrf_configure (dev, NRF_CONFIGURATION_REGISTER_SIZE);

    /* Start receiving.  */
    nrf_enable (dev);

    for (i = 0; i < ms_to_wait && !NRF_DATA_READY_P (dev); i++)
        delay_ms (1);

    if (!NRF_DATA_READY_P (dev))
        return 0;

    /* Data is available, clock it out into the buffer.  */
    packet_length = nrf_payload_length1_get (dev);
    if (len > packet_length)
        len = packet_length;
    spi_read (dev->spi, buffer, len, 1);

    nrf_standby (dev);

    return packet_length;
}


/* Transmit a packet in ShockBurst mode without any retransmission.  */
uint8_t
nrf_transmit (nrf_t nrf, nrf_node_t *node, void *buffer, uint8_t len)
{
    nrf_obj_t *dev = nrf;
    uint8_t i;
    uint8_t length;
    uint16_t packet_length;
    uint8_t *data = buffer;
    uint8_t delay;

    /* Changing channel requires a one byte configure.  */
    nrf_channel_set (dev, node->tx.channel);

    /* Changing direction requires a one byte configure.  */
    nrf_dir_set (dev, NRF_TX_MODE);

    nrf_configure (dev, 1);

    /* Clock out a packet to the nRF chip, with zero-padding if len is
       less than the packet length.  */

    /* Set the chip enable pin high to begin clocking in data.  */
    NRF_CE_HIGH_SET (dev);

    DELAY_US (NRF_LINE_TIME_ENABLE_US);

    /* Set length to address length in bytes.  */
    length = (dev->config.bits.addr_w / CHAR_BIT);

    /* Clock out address.  */
    for (i = length; i > 0; i--)
        spi_putc (dev->spi, node->tx.address.bytes[i - 1]);

    /* Set length to payload length in bytes.  */
    length = (dev->config.bits.data1_w / CHAR_BIT);

    /* Clock out data.  */
    for (i = 0; i < len; i++)
        spi_putc (dev->spi, data[i]);

    /* If size is smaller than the payload length, append zeros.  */
    if (len < length)
    {
        for (i = 0; i < (length - len); i++)
            spi_putc (dev->spi, 0);
    }

    /* Set the chip enable pin low to transmit data and return to
       standby mode.  */
    NRF_CE_LOW_SET (dev);

    /* Delay to allow chip to enter ShockBurst mode.  */
    DELAY_US (NRF_T_SB_ACTIVE);

    /* Delay to allow for transmission time.  */
    
    /* Transmission delay time depends on many factors.  */
    /* The packet length = address length + data length + crc.  */
    packet_length = ((dev->config.bits.addr_w) + (dev->config.bits.data1_w));
    /* If CRC is enabled, add its length on.  */
    if (dev->config.bits.crc_en == NRF_CRC_ENABLED)
    {
        if (dev->config.bits.crc_l == NRF_CRC_8) /* 8-bit CRC enabled.  */
        {
            packet_length += 8;
        }
        else /* 16-bit CRC enabled.  */
        {
            packet_length += 16;
        }
    }

    /* If data rate = 1Mbit/s, delay for a number of microseconds
       equal to the packet length.  Otherwise, if data rate = 250kbps,
       delay for 4 times as long.  Now we don't want to calculate
       microsecond delays since this requires floating point
       arithmetic.  */
    delay = packet_length;
    if (dev->config.bits.rfdr_sb != NRF_DATA_1M)
        delay <<= 2;
    while (delay)
    {
        DELAY_US (1);
        delay--;
    }

    /* The chip automatically returns to standby mode after transmission.  */
    
    return len;
}


bool 
nrf_data_ready_p (nrf_t nrf)
{
    nrf_obj_t *dev= nrf;

    return NRF_DATA_READY_P (dev);
}


bool
nrf_setup (nrf_t nrf, uint8_t payload_size)
{
    nrf_obj_t *dev= nrf;

    nrf_dir_set (dev, NRF_RX_MODE);
    nrf_channel_set (dev, 0);

    nrf_power_set (dev, NRF_POWER_0);
    nrf_xtal_freq_set (dev, NRF_XTAL_FREQ_16M);
    nrf_data_rate_set (dev, NRF_DATA_1M);
    nrf_comms_mode_set (dev, NRF_SHOCKBURST);
    nrf_single_or_dual_channel_set (dev, NRF_SINGLE_CHANNEL);
    nrf_crc_status_set (dev, NRF_CRC_ENABLED);
    nrf_crc_length_set (dev, NRF_CRC_16);
    nrf_address_length_set (dev, (CHAR_BIT * sizeof (nrf_address_t)));
    nrf_payload_length1_set (dev, CHAR_BIT * payload_size);

    nrf_configure (dev, NRF_CONFIGURATION_REGISTER_SIZE);

    return 1;
}

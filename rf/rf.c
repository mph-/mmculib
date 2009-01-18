/** @file   rf.c
    @author Tony Culliford / Michael Hayes
    @date   8 December 2004

    @brief Higher-level routines for interfacing to a wireless link.
*/

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "rf.h"
#include "time.h"
#include "delay.h"
#include "port.h"

#ifndef RF_DEBUG
#define RF_DEBUG 0
#endif

#ifndef HIGH_BYTE
#define HIGH_BYTE(x)  ((x) >> 8)
#endif

#ifndef LOW_BYTE
#define LOW_BYTE(x)  ((x) & 0xFF)
#endif


#if RF_DEBUG
static void
rf_debug_address (rf_address_t *address)
{
    uint8_t i;

    printf ("[");
    for (i = 0; i < sizeof (*address); i++)
        printf ("%02x ", address->bytes[i]);
    printf ("]");
}


static void
rf_debug_packet (uint8_t *data, rf_size_t size)
{
    rf_size_t i;

    printf ("{");
    for (i = 0; i < size; i++)
        printf ("%02x ", data[i]);
    printf ("}");
}
#endif


static void
rf_address_make (rf_address_t *address, rf_id_t id)
{
    rf_address_t default_address = {NRF_DEFAULT_ADDRESS};

    memcpy (address, &default_address, sizeof (*address));
    address->bytes[0] = id;
}


void
rf_node_make (rf_node_t *node, rf_id_t id, uint8_t channel)
{
    node->id = id;
    node->channel = channel;
}


void
rf_write_setup (rf_t rf, rf_node_t *dst_node)
{
    rf_obj_t *dev = rf;

#if RF_DEBUG
    printf ("Tx setup %d:%d\r\n",
            dst_node->id, dst_node->channel);
#endif

    /* This will do nothing if the channel doesn't need changing
       otherwise it requires a one byte configure.  */
    RF_DEVICE_CHANNEL_SET (dev, dst_node->channel);

    /* Change to transmit mode.  */
    RF_TX_MODE_SET (dev);
}


void
rf_standby (rf_t rf)
{
    rf_obj_t *dev = rf;

    RF_DEVICE_DISABLE (dev);
}


/* Unreliable packet transmission.  */
rf_size_t
rf_transmit (rf_t rf, rf_node_t *dst_node, uint8_t *data, rf_size_t size)
{
    rf_address_t address;

    rf_address_make (&address, dst_node->id);

#if RF_DEBUG && 0
    printf ("Tx %d:%d (%d) ",
            dst_node->id, dst_node->channel, size);
    rf_debug_address (&address);
    rf_debug_packet (data, size);
    printf ("\r\n");
#endif

    return RF_TRANSMIT (rf, &address, data, size);
}


void
rf_read_enable (rf_t rf)
{
    rf_obj_t *dev = rf;

    /* Set the transceiver into receive mode.  This requires
       programming of the transceiver.  */
    RF_RX_MODE_SET (dev);

    /* Start receiving.  */
    RF_DEVICE_ENABLE (dev);
}


void
rf_read_setup (rf_t rf, rf_node_t *node)
{
    rf_obj_t *dev = rf;
    rf_address_t address;

    rf_address_make (&address, node->id);

#if RF_DEBUG
    printf ("Rx setup %d:%d ",
            node->id, node->channel);
    rf_debug_address (&address);
    printf ("\r");
#endif

    /* Changing the address requires a full configure. */
    RF_DEVICE_ADDRESS_SET (dev, &address);

    /* This will do nothing if the channel doesn't need changing
       otherwise it requires a one byte configure.  */
    RF_DEVICE_CHANNEL_SET (dev, node->channel);

    /* Set the transceiver into receive mode.  This will do nothing if
       already in receive mode otherwise it requires a one byte
       configure.  */
    RF_RX_MODE_SET (dev);

    /* Start receiving.  */
    RF_DEVICE_ENABLE (dev);
}


/* Unreliable packet reception.  */
rf_size_t
rf_receive (rf_t rf, uint8_t *data, uint8_t ms_to_wait)
{
    rf_size_t ret;

    ret = RF_RECEIVE (rf, data, ms_to_wait);

#if RF_DEBUG
    printf ("Rx (%d) ", ret);
    rf_debug_packet (data, ret);
    printf ("\r\n");
#endif

    return ret;
}


/* Function to send a block of data to a recipient, with re-transmits.  */
rf_size_t
rf_write (rf_t rf, rf_node_t *dst_node, uint8_t *data, rf_size_t size)
{
    uint8_t bytes_to_send;
    uint8_t packet_success;
    uint8_t attempts_num;
    rf_size_t i;
    rf_obj_t *dev = rf;

    for (i = 0; i < size; i += RF_PAYLOAD_SIZE)
    {
        packet_success = 0;
        attempts_num = 0;

        /* Retransmit until packet successfully received.  */
        while (!packet_success && attempts_num < RF_RETRIES_MAX)
        {
            /* Set the transceiver into transmit mode.  */
            rf_write_setup (rf, dst_node);

            /* Calculate number of bytes in this packet.  */
            if ((size - i) > RF_PAYLOAD_SIZE)
                  bytes_to_send = RF_PAYLOAD_SIZE;
              else
                  bytes_to_send = (size - i);
  
            /* Transmit a packet.  */
            rf_transmit (dev, dst_node, (data + i), bytes_to_send);
            attempts_num++;

            /* Wait for acknowledgement.  */
            packet_success = 
                rf_acknowledge_wait (dev, dst_node, *(data + i), 0);
        }
    }
    return i;
}



/* Function to receive a packet of data.  Waits up to the specified
   number of milliseconds for data to arrive.  rf_read_setup must
   be previously called.  */
rf_size_t
rf_read (rf_t rf, uint8_t *data, rf_size_t size, uint8_t ms_to_wait)
{
    /* Should check size against payload size...  */
    return rf_receive (rf, data, ms_to_wait);
}



/* Function to initialise and then put RF chip into standby.  */
rf_t
rf_setup (rf_obj_t *dev, rf_cfg_t *cfg)
{
    /* Initialise address.  */

    /* Put the device into standby, ready for programming.  */
    RF_DEVICE_DISABLE (dev);
   
    /* Initialise device.  */
    RF_INIT (dev, cfg);

    /* Set up RF unit.  */
    RF_SETUP (dev, RF_PAYLOAD_SIZE);

    RF_DEVICE_DISABLE (dev);
    return dev;
}



uint8_t
rf_acknowledge_wait (rf_t rf, rf_node_t *node, 
                     uint8_t command, time_t *timestamp)
{
    rf_obj_t *dev = rf;
    uint8_t ret;
    uint8_t rx_buffer[RF_PAYLOAD_SIZE];


    rf_read_enable (rf);

    ret = rf_receive (dev, rx_buffer, NRF_TIME_OUT_ACK_MS);

    if (!ret)
    {
        /* No packet received.  */
        return 0;
    }
    else if (rx_buffer[0] != RF_CMD_ACK)
    {
        /* Received packet not an ACK.  */
        return 0;
    }
    else if (rx_buffer[5] != command)
    {
        /* ACK for wrong command.  */
        return 0;
    }

    /* Will check ACK address later -- FIXME.  */

    if (timestamp)
    {
        timestamp->ms_ticks = (rx_buffer[1] << 8) | rx_buffer[2];
        timestamp->us_ticks = (rx_buffer[3] << 8) | rx_buffer[4];
    }

    return 1;
}



void
rf_acknowledge (rf_t rf, rf_node_t *dst_node, uint8_t command)
{
    rf_obj_t *dev = rf;
    uint8_t tx_buffer[6];
    time_t timestamp;

    timestamp = time_rf_timestamp_get ();

    tx_buffer[0] = RF_CMD_ACK;
    tx_buffer[1] = HIGH_BYTE (timestamp.ms_ticks);
    tx_buffer[2] = LOW_BYTE (timestamp.ms_ticks);
    tx_buffer[3] = HIGH_BYTE (timestamp.us_ticks);
    tx_buffer[4] = LOW_BYTE (timestamp.us_ticks);
    tx_buffer[5] = command;

    rf_write_setup (rf, dst_node);
    
    /* Delay for the length of time required by the protocol before
       acknowledging.  */
    DELAY_US (NRF_ACK_DELAY_US);
    
    /* Transmit the acknowledgement.  */
    rf_transmit (dev, dst_node, tx_buffer, sizeof (tx_buffer));
}



rf_t
rf_init (rf_obj_t *dev, rf_cfg_t *cfg)
{
    /* Initialise all the systems required for a wireless link.  */

    rf_setup (dev, cfg);
            
    return dev;
}



/* Function to send a block of data to a recipient, with re-transmits.  */
uint16_t
rf_write_data (rf_t rf, rf_node_t *dst_node, uint8_t *data, uint16_t size)
{
    rf_obj_t *dev = rf;
    uint8_t packet_number = 0;
    uint8_t ret;
    uint16_t i;
    uint8_t  j;
    uint8_t tx_buffer[RF_PAYLOAD_SIZE];
    uint8_t rx_buffer[RF_PAYLOAD_SIZE];

    /* Handshake the data transfer.  */
    tx_buffer[0] = HIGH_BYTE (size);
    tx_buffer[1] = LOW_BYTE (size);
    rf_command_no_ack (dev, dst_node, RF_CMD_DATA_START, tx_buffer, 2);

    /* Need to add a delay here to allow the command to be recieved
     * before spitting out the data packets. Otherwise the first data
     * packet will be lost.  */
    delay_ms (2);

    /* Delay to allow receiving unit to get ready.  */
    DELAY_US (RF_READY_WAIT_US);

    /* Packets consist of 1 index byte and RF_PAYLOAD_SIZE - 1 data bytes.  */
    for (i = 0; i < size; i += (RF_PAYLOAD_SIZE - 1))
    {
        tx_buffer[0] = packet_number;
        packet_number++;

        /* Populate transmit buffer.  */
        for (j = 0; j < (RF_PAYLOAD_SIZE - 1); j++)
            tx_buffer[j + 1] = data[i + j];

        rf_transmit (dev, dst_node, tx_buffer, RF_PAYLOAD_SIZE);

        /* Delay to allow receiving unit to get ready.  */
        DELAY_US (RF_READY_WAIT2_US);
        /* Hack to extend time delay & allow handunit to keep up
         */
        DELAY_US (500);
        DELAY_US (500);
    }

    ret = 0;
    do
    {
        rf_read_enable (dev);
        /* Wait much longer than standard delay
           Alow for the handunit to send 3 requests for a packet
           and that the first 2 are lost.  */
        ret = rf_receive (dev, rx_buffer, (NRF_TIME_OUT_ACK_MS * 3 + 2));

        if (ret)
        {
            if (rx_buffer[0] == RF_CMD_ALL_PACKETS_RECEIVED)
            {
                rf_acknowledge (dev, dst_node, rx_buffer[0]);
                return size; 
            }
            
            else if (rx_buffer[0] == RF_CMD_RESEND_PACKET)
            {
                rf_acknowledge (dev, dst_node, rx_buffer[0]);
                
                /* Do not send the packet immediately after acknowledge
                   or one of the two may not be recieved */
                delay_ms (2);

                /* Number of packet to re-transmit.  */
                tx_buffer[0] = rx_buffer[1];

                /* Populate transmit buffer.  */
                for (j = 0; j < (RF_PAYLOAD_SIZE - 1); j++)
                    tx_buffer[j + 1] = data[((RF_PAYLOAD_SIZE - 1)
                                             * rx_buffer[1]) + j];
                
                /* Do not wait for an aknowledgement. The master
                   will re-request the packet if required.  */
                rf_transmit (dev, dst_node, tx_buffer, RF_PAYLOAD_SIZE);
            }
        }
    }
    while (ret);
    
    return 0;           // Failure
}



bool
rf_read_ready_p (rf_t rf)
{
    return RF_DATA_READY_P (rf);
}



/* Reliable transmission of an rf command
   Uses rf_write to send command which will retry until an
   acknowledgement is recieved.  */
/* *data should contain any payload information required by the command
   data_size should be the number of bytes of data to be transmitted
   with the command.  */
rf_size_t 
rf_command (rf_t rf, rf_node_t *dst_node, rf_cmd_t command,
            void *data, rf_size_t data_size)
{
    uint8_t buffer[RF_PAYLOAD_SIZE];

    buffer[0] = command;
    memcpy (buffer + 1, data, data_size);

    return rf_write (rf, dst_node, buffer, data_size + 1);
}



/* Unreliable transmission of an rf command.  */
/* *data should contain any payload information required by the command
   data_size should be the number of bytes of data to be transmitted
   with the command.  */
void 
rf_command_no_ack (rf_t rf, rf_node_t *dst_node, rf_cmd_t command,
                   void *data, rf_size_t data_size)
{
    uint8_t buffer[RF_PAYLOAD_SIZE];

    buffer[0] = command;
    memcpy (buffer + 1, data, data_size);

    rf_write_setup (rf, dst_node);

    rf_transmit (rf, dst_node, buffer, data_size + 1);
}

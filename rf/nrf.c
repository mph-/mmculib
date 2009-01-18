#include <string.h>
#include "nrf.h"
#include "delay.h"


static nrf_node_t broadcast_node =
{
    {NRF_MASTER_ID, {NRF_DEFAULT_ADDRESS}, 1},
    {0, {NRF_DEFAULT_ADDRESS}, 1}
};


#if 0
void 
nrf_node_create (nrf_node_t *node, 
                 nrf_address_t tx_address, nrf_address_t rx_address,
                 nrf_channel_t tx_channel, nrf_channel_t rx_channel,
                 nrf_id_t tx_id, nrf_id_t rx_id)
{
    (node->tx.address) = tx_address;
    (node->rx.address) = rx_address;
    node->tx.channel = tx_channel;
    node->rx.channel = rx_channel;
    node->rx.id = rx_id;
    node->tx.id = tx_id;
}
#endif


void 
nrf_broadcast_node_create (nrf_node_t *node)
{
    *node = broadcast_node;
}


/* Function to send a block of data to a recipient, with re-transmits.  */
nrf_size_t
nrf_write (nrf_t rf, nrf_node_t *node, void *buffer, nrf_size_t len)
{
    nrf_size_t i;
    uint8_t *data = buffer;
    nrf_obj_t *dev = rf;

    for (i = 0; i < len; i += NRF_PAYLOAD_SIZE)
    {
        uint8_t packet_success;
        uint8_t attempts_num;

        packet_success = 0;
        attempts_num = 0;

        /* Retransmit until packet successfully received.  */
        while (!packet_success && attempts_num < NRF_RETRIES_MAX)
        {
            uint8_t bytes_to_send;

            /* Calculate number of bytes in this packet.  */
            if ((len - i) > NRF_PAYLOAD_SIZE)
                  bytes_to_send = NRF_PAYLOAD_SIZE;
              else
                  bytes_to_send = (len - i);
  
            /* Transmit a packet.  */
            nrf_transmit (dev, node, (data + i), bytes_to_send);
            attempts_num++;

            /* Wait for acknowledgement.  */
            packet_success = 
                nrf_acknowledge_wait (dev, node, *(data + i), 0, 0);
        }
    }
    return i;
}


/* Function to receive a packet of data.  Waits up to the specified
   number of milliseconds for data to arrive.  nrf_read_setup must
   be previously called.  */
nrf_size_t
nrf_read (nrf_t rf, nrf_node_t *node, void *data, nrf_size_t len, 
          uint8_t ms_to_wait)
{
    /* Should check size against payload size...  */
    return nrf_receive (rf, node, data, len, ms_to_wait);
}


uint8_t
nrf_acknowledge_wait (nrf_t rf, nrf_node_t *node, 
                      nrf_cmd_t command, void *buffer, uint8_t len)
{
    nrf_obj_t *dev = rf;
    uint8_t ret;
    uint8_t rx_buffer[NRF_PAYLOAD_SIZE];

    ret = nrf_receive (dev, node, rx_buffer, NRF_PAYLOAD_SIZE,
                       NRF_TIMEOUT_ACK_MS);

    if (!ret)
    {
        /* No packet received.  */
        return 0;
    }
    else if (rx_buffer[0] != NRF_CMD_ACK)
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

    if (buffer)
        memcpy (buffer, rx_buffer + 1, len);

    return 1;
}


void
nrf_acknowledge (nrf_t rf, nrf_node_t *node, nrf_cmd_t command,
                 void *buffer, uint8_t len)
{
    nrf_obj_t *dev = rf;
    uint8_t tx_buffer[NRF_PAYLOAD_SIZE];

    tx_buffer[0] = NRF_CMD_ACK;
    tx_buffer[1] = command;
    if (buffer)
        memcpy (tx_buffer + 2, buffer, len);
    
    /* Delay for the length of time required by the protocol before
       acknowledging.  */
    DELAY_US (NRF_ACK_DELAY_US);
    
    /* Transmit the acknowledgement.  */
    nrf_transmit (dev, node, tx_buffer, len + 2);
}

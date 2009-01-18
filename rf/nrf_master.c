/** @file   nrf_master.c
    @author Michael Hayes
    @date
    @brief 
*/

#include <string.h>
#include "nrf.h"
#include "delay.h"
#include "target.h"


#define NRF_DEVICE_RESPONSE_WAIT_MS 100
#define NRF_DEVICE_RESPONSE_WAIT_NEXT_MS 5
#define NRF_ENUMERATE_REPEAT_NUM 3
#define NRF_UNACK_CMD_REPEAT_NUM 3
#define NRF_UNACK_CMD_REPEAT_DELAY_MS 2



/* Assumes all devices are listening on the broadcast channel.  Returns
   the number of devices located.  */
uint8_t
nrf_devices_enumerate (nrf_t nrf, nrf_device_t *devices, uint8_t devices_max)
{
    nrf_obj_t *dev = nrf;
    uint8_t i;
    uint8_t j;
    uint8_t ret;
    uint8_t device_index;
    uint8_t device_num;
    uint8_t *device_id;
    uint8_t buffer[NRF_PAYLOAD_SIZE];
    nrf_node_t master_node;
    nrf_node_t slave_node;

    nrf_node_make (&master_node, NRF_MASTER_ID, NRF_BROADCAST_CHANNEL);
    nrf_node_make (&slave_node, NRF_BROADCAST_SLAVE_ID, NRF_BROADCAST_CHANNEL);

    /* Configure for a read before transmit so we get a quick turnaround
       into receive mode.  */
    nrf_read_setup (nrf, &master_node);

    for (device_num = 0; device_num < devices_max; device_num++)
    {
        /* Try NRF_ENUMERATE_REPEAT_NUM times to guard against channel
           loss.  */
        for (i = 0; i < NRF_ENUMERATE_REPEAT_NUM; i++)
        {
            nrf_device_t *device;

            /* Calling all cars...  */
            nrf_command_no_ack (dev, &slave_node, NRF_CMD_DEVICE_ID_GET,
                               NULL, 0);

            nrf_read_enable (nrf);

            /* Wait for device responses, until NRF_DEVICE_RESPONSE_WAIT_MS
               passes with no new device responding.  */
            ret = nrf_read (dev, buffer, sizeof (buffer),
                           NRF_DEVICE_RESPONSE_WAIT_MS);

            if (!ret)
                continue;
            
            if (buffer[0] != NRF_CMD_DEVICE_ID_RESPONSE)
                continue;

            device_id = buffer + 1;
            
            /* Look to see if the device has already been enumerated.  
               This should not happen.  */
            device_index = nrf_devices_search (device_id, devices, device_num);
            if (device_index != NRF_DEVICE_NOT_FOUND)
                continue;
            
            /* Need to send the device_id & the device_num in the
               enumeration command. The device ID is still in buffer,
               so just append the device_num.  */
            buffer[NRF_DEVICE_ID_SIZE + 1] = device_num;

            /* Since this next command is never acknowledged or replied
               to in any way, repeat to guard against packet loss.  */
            for (j = 0; j < NRF_UNACK_CMD_REPEAT_NUM; j++)
            {
                nrf_command_no_ack (dev, &slave_node, NRF_CMD_ENUMERATE_DEVICE,
                        device_id, NRF_DEVICE_ID_SIZE + 1);
                
                delay_ms (NRF_UNACK_CMD_REPEAT_DELAY_MS);
            }
   
            /* Store device IDs.  */
            device = devices + device_num;
            memcpy (device->device_id, device_id, NRF_DEVICE_ID_SIZE);
            device->node.id = device_num;
            device->node.channel = NRF_BROADCAST_CHANNEL;
            break;
        }

        /* If nothing has responded after NRF_ENUMERATE_REPEAT_NUM
           attempts then there are no more devices to enumerate.  */
        if (i == NRF_ENUMERATE_REPEAT_NUM)
            return device_num;
    }
    
    return device_num;
}


uint16_t
nrf_read_data (nrf_t nrf, nrf_node_t *node, uint8_t *data, uint16_t size)
{
    nrf_obj_t *dev = nrf;
    uint8_t i;
    uint8_t j;
    uint8_t ret, packets_num;
    uint8_t rx_buffer[NRF_PAYLOAD_SIZE];
    uint8_t packet_received_flags[32] = { 0 };

    /* Size bytes to be received.  */
    packets_num = size / (NRF_PAYLOAD_SIZE - 1);

    do
    {
        /* The nrf_recieve command leaves the unit in standby mode
           Return it to rx mode for the next packet */
        nrf_read_enable (dev);

        /* DEBUG: wait delay extended by 2 ms */
        ret = nrf_read (dev, rx_buffer, sizeof (rx_buffer),
                        NRF_READ_WAIT_MS + 2); 

        if (ret)
        {
            for (i = 0; i < (NRF_PAYLOAD_SIZE - 1); i++)
                data[((NRF_PAYLOAD_SIZE - 1) * rx_buffer[0]) + i]
                    = rx_buffer[i + 1];

            packet_received_flags[rx_buffer[0]] = 1;
        }
    }
    while (ret);


    /* The nrf_recieve command leaves the unit in standby mode
       Return it to rx mode for the next packet.  */
        
    /* Loop through to find out if any packets are missing.  */
    for (i = 0; i < packets_num; i++)
    {
        if (packet_received_flags[i] == 0)
        {
            ret = 0;

            /* Request re-transmission.  */
            ret = nrf_command (dev, node, NRF_CMD_RESEND_PACKET, &i, 1);

            /* Failure to get a necessary packet.
             * Only the packets recieved up to this point are useable.  */
            if (!ret)
            {
                return i;
            }
            
            ret = 0;
            
            do
            {
                nrf_read_enable (dev);

                /* Device waits 2ms before transmitting the packet
                   so timeout after 5ms  */
                ret = nrf_read (dev, rx_buffer, sizeof (rx_buffer), 5);

                if (ret)
                {
                    /* Device is not expecting an acknowledgement  */
            
                    for (j = 0; j < (NRF_PAYLOAD_SIZE - 1); j++)
                        data[((NRF_PAYLOAD_SIZE - 1) * rx_buffer[0]) + j] = rx_buffer[j + 1];
                }
            }
            while (ret);
        }
    }

    nrf_command (dev, node, NRF_CMD_ALL_PACKETS_RECEIVED, NULL, 0);
    return size;
}

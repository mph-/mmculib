/** @file  rf_master.c
    @author MPH

    @brief Stripped from original rf file wireless.c to handle
   rf functions only used by the master node.
*/

#include <string.h>
#include "rf.h"
#include "delay.h"

#include "pio.h"
#include "target.h"


#define RF_PROBE_RESPONSE_WAIT_MS 100
#define RF_PROBE_RESPONSE_WAIT_NEXT_MS 5
#define RF_ENUMERATE_REPEAT_NUM 3
#define RF_UNACK_CMD_REPEAT_NUM 3
#define RF_UNACK_CMD_REPEAT_DELAY_MS 2



/* Searches the currently identified list of probes to see whether a
   given device ID is present.  Returns the probe number of the found
   probe.  */
static uint8_t
rf_probes_search (uint8_t *device_id, rf_probe_t *probes, uint8_t probes_num)
{
    uint8_t i;

    for (i = 0; i < probes_num; i++)
    {
        if (memcmp (probes[i].device_id, device_id, RF_DEVICE_ID_SIZE) == 0)
            return i;
    }
    return RF_PROBE_NOT_FOUND;
}


/* Assumes all devices are listening on the broadcast channel.  Returns
   the number of probes located.  */
uint8_t
rf_probes_enumerate (rf_t rf, rf_probe_t *probes, uint8_t probes_max)
{
    rf_obj_t *dev = rf;
    uint8_t i;
    uint8_t j;
    uint8_t ret;
    uint8_t probe_index;
    uint8_t probe_num;
    uint8_t *device_id;
    uint8_t buffer[RF_PAYLOAD_SIZE];
    rf_node_t master_node;
    rf_node_t slave_node;

    rf_node_make (&master_node, RF_MASTER_ID, RF_BROADCAST_CHANNEL);
    rf_node_make (&slave_node, RF_BROADCAST_SLAVE_ID, RF_BROADCAST_CHANNEL);

    /* Configure for a read before transmit so we get a quick turnaround
       into receive mode.  */
    rf_read_setup (rf, &master_node);

    for (probe_num = 0; probe_num < probes_max; probe_num++)
    {
        /* Try RF_ENUMERATE_REPEAT_NUM times to guard against channel
           loss.  */
        for (i = 0; i < RF_ENUMERATE_REPEAT_NUM; i++)
        {
            rf_probe_t *probe;

            /* Calling all cars...  */
            rf_command_no_ack (dev, &slave_node, RF_CMD_DEVICE_ID_GET,
                               NULL, 0);

            rf_read_enable (rf);

            /* Wait for probe responses, until RF_PROBE_RESPONSE_WAIT_MS
               passes with no new probe responding.  */
            ret = rf_read (dev, buffer, sizeof (buffer),
                           RF_PROBE_RESPONSE_WAIT_MS);

            delay_us (100);
            
            if (!ret)
                continue;
            
            if (buffer[0] != RF_CMD_DEVICE_ID_RESPONSE)
                continue;

            device_id = buffer + 1;
            
            /* Look to see if the probe has already been enumerated.  
               This should not happen.  */
            probe_index = rf_probes_search (device_id, probes, probe_num);
            if (probe_index != RF_PROBE_NOT_FOUND)
                continue;
            
            /* Need to send the device_id & the probe_num in the
               enumeration command. The device ID is still in buffer,
               so just append the probe_num.  */
            buffer[RF_DEVICE_ID_SIZE + 1] = probe_num;

            /* Since this next command is never acknowledged or replied
               to in any way, repeat to guard against packet loss.  */
            for (j = 0; j < RF_UNACK_CMD_REPEAT_NUM; j++)
            {
                rf_command_no_ack (dev, &slave_node, RF_CMD_ENUMERATE_DEVICE,
                        device_id, RF_DEVICE_ID_SIZE + 1);
                
                delay_ms (RF_UNACK_CMD_REPEAT_DELAY_MS);
            }
   
            /* Store probe IDs.  */
            probe = probes + probe_num;
            memcpy (probe->device_id, device_id, RF_DEVICE_ID_SIZE);
            probe->node.id = probe_num;
            probe->node.channel = RF_BROADCAST_CHANNEL;
            break;
        }

        /* If nothing has responded after RF_ENUMERATE_REPEAT_NUM
           attempts then there are no more probes to enumerate.  */
        if (i == RF_ENUMERATE_REPEAT_NUM)
            return probe_num;
    }
    
    return probe_num;
}


uint16_t
rf_read_data (rf_t rf, rf_node_t *node, uint8_t *data, uint16_t size)
{
    rf_obj_t *dev = rf;
    uint8_t i;
    uint8_t j;
    uint8_t ret, packets_num;
    uint8_t rx_buffer[RF_PAYLOAD_SIZE];
    uint8_t packet_received_flags[32] = { 0 };

    /* Size bytes to be received.  */
    packets_num = size / (RF_PAYLOAD_SIZE - 1);

    do
    {
        /* The nrf_recieve command leaves the unit in standby mode
           Return it to rx mode for the next packet */
        rf_read_enable (dev);

        /* DEBUG: wait delay extended by 2 ms */
        ret = rf_read (dev, rx_buffer, sizeof (rx_buffer), RF_READ_WAIT_MS + 2); 

        if (ret)
        {
            for (i = 0; i < (RF_PAYLOAD_SIZE - 1); i++)
                data[((RF_PAYLOAD_SIZE - 1) * rx_buffer[0]) + i]
                    = rx_buffer[i + 1];

            packet_received_flags[rx_buffer[0]] = 1;
        }
    }
    while (ret);

    // DEBUG: Print flags
    /*printf ("\rpacket_recieved_flags = \r");
    for (i = 0; i < 32; i++)
        printf ("%x, ", packet_received_flags[i]); 
    printf ("\r\r");
    */


    /* The nrf_recieve command leaves the unit in standby mode
       Return it to rx mode for the next packet */
        
    /* Loop through to find out if any packets are missing.  */
    for (i = 0; i < packets_num; i++)
    {
        if (packet_received_flags[i] == 0)
        {
            ret = 0;

            /* Request re-transmission.  */
            ret = rf_command (dev, node, RF_CMD_RESEND_PACKET, &i, 1);

            /* Failure to get a necessary packet.
             * Only the packets recieved up to this point are useable.  */
            if (!ret)
            {
                return i;
            }
            
            ret = 0;
            
            do
            {
                rf_read_enable (dev);

                /* Probe waits 2ms before transmitting the packet
                   so timeout after 5ms  */
                ret = rf_read (dev, rx_buffer, sizeof (rx_buffer), 5);

                if (ret)
                {
                    /* Probe is not expecting an acknowledgement  */
                    //rf_acknowledge (dev, node, rx_buffer[0]);
            
                    for (j = 0; j < (RF_PAYLOAD_SIZE - 1); j++)
                        data[((RF_PAYLOAD_SIZE - 1) * rx_buffer[0]) + j] = rx_buffer[j + 1];
                }
            }
            while (ret);
        }
    }

    rf_command (dev, node, RF_CMD_ALL_PACKETS_RECEIVED, NULL, 0);
    return size;
}

/** @file   rf_slave.c
    @author Michael Hayes
    @date   25 March 2005

    @brief Slave only wireless routines.
*/

#include <string.h>
#include "rf.h"
#include "delay.h"


enum {RF_PROBE_ENUMERATE_WAIT_MS = 5};


/* A function to control how probes respond to enumeration requests.  */
rf_id_t
rf_enumeration_response (rf_t rf, uint8_t *device_id, uint8_t *version)
{
    rf_obj_t *dev = rf;
    rf_id_t rf_id;
    rf_size_t ret;
    uint8_t i, j;
    uint8_t *buf;
    uint8_t buffer[RF_PAYLOAD_SIZE];
    rf_node_t slave_node;
    rf_node_t master_node;

    rf_node_make (&master_node, RF_MASTER_ID, RF_BROADCAST_CHANNEL);

    /* Listen on the broadcast address.  */
    rf_node_make (&slave_node, RF_BROADCAST_SLAVE_ID, RF_BROADCAST_CHANNEL);
    rf_read_setup (rf, &slave_node);

    /* Try each of the device ID bytes until no conflict with another
       slave.  */
    for (i = 0; i < RF_DEVICE_ID_SIZE; i++)
    {
        /* Wait a number of ms equal to the low 4 bits of its ID + 2
           ms before responding.  This is to reduce the chances of
           collision.  */
        delay_ms ((device_id[i] & 0x0F) + 2);

        /* Populate buf with ID & version for transmission.  */
        buf = buffer;
        for (j = 0; j < RF_DEVICE_ID_SIZE; j++)
            *buf++ = device_id[j];
        for (j = 0; version[j]; j++)  // Don't understand while(version[j]) :SW
            *buf++ = version[j];      // version is a uint8?!?

        /* Respond to the master unit with the probe's ID.  */
        rf_command_no_ack (rf, &master_node, RF_CMD_DEVICE_ID_RESPONSE,
                    buffer, RF_PAYLOAD_SIZE - 1);

        rf_read_enable (rf);

       /* Wait up to RF_PROBE_ENUMERATE_WAIT_MS to see if the master
           unit acknowledges and enumerates.  */
        ret = rf_read (dev, buffer, sizeof (buffer),
                       RF_PROBE_ENUMERATE_WAIT_MS);

        if (ret > 0 && buffer[0] == RF_CMD_ENUMERATE_DEVICE)
        {
            /* Check that the enumeration is intended for this device. 
               If not another device responded first so give up
               and wait for the next RF_CMD_ID_GET request.  */
            if (memcmp (buffer + 1, device_id, RF_DEVICE_ID_SIZE) != 0)
                return RF_BROADCAST_SLAVE_ID;

            rf_id = buffer[RF_DEVICE_ID_SIZE + 1];

            /* Acknowledge packet reception.  */
            rf_acknowledge (dev, &master_node, buffer[0]);

            /* Return the enumeration of this node.  */
            return rf_id;
        }
    }

    /* Something went wrong, keep on the broadcast channel.  */
    return RF_BROADCAST_SLAVE_ID;
}

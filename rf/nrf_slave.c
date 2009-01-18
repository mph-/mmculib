/** @file   nrf_slave.c
    @author Michael Hayes
    @date   25 March 2005
    @brief  Slave only wireless routines for Nordic rf chips.
*/

#include <string.h>
#include "nrf.h"
#include "delay.h"


/* A slave device can shout out on the broadcast address and broadcast
   channel then listen on the broadcast address and broadcast channel
   for an enumeration message from the master assigning the address
   and channel to use.

   An alternative approach would be for each slave to listen on the
   broadcast address and channel.  The master would periodically
   transmit asking for any new devices out there.  The slaves would
   then reply (backing off as necessary to avoid conflicts).  This
   method has the advantage that the master initiates all
   transfers.  */


/* Announce presence of device and wait for enumeration acknowledgement.  */
bool
nrf_announce (nrf_t nrf, nrf_node_t *node, uint8_t *device_id)
{
    nrf_obj_t *dev = nrf;
    uint8_t j;
    uint8_t command[sizeof (nrf_enumerate_t)];
    nrf_enumerate_t *response;

    /* Create a new node; this will be assigned the broadcast address
       and broadcast channel.  */
    nrf_broadcast_node_create (node);
    
    command[0] = NRF_CMD_ANNOUNCE;
    for (j = 0; j < NRF_DEVICE_ID_SIZE; j++)
        command[j + 1] = device_id[j];

    nrf_transmit (dev, node, command, j + 1);

    if (!nrf_receive (nrf, node, command, 16, 20))
        return 0;

    response = (void *)command;

    if (response->cmd != NRF_CMD_ENUMERATE)
        return 0;

    /* Check if response meant for us.  */
    if (memcmp (response->device_id, device_id, NRF_DEVICE_ID_SIZE))
        return 0;

    /* Store link designations to use.  */
    node->tx = response->tx;
    node->rx = response->rx;

    /* Send confirmation on the new link designation.  */
    command[0] = NRF_CMD_ENUMERATE_ACK;
    command[1] = response->rx.id;
    nrf_transmit (dev, node, command, 2);

    return 1;
}

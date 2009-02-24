/** @file flashheap.c
 *  @author Michael Hayes
 *  @date 23 February 2009
 * 
 *  @brief Routines to implement a heap in a dataflash memory.
 */

#include <flashheap.h>
#include <stdlib.h>

/* The goals of this code is to implement a heap within flash memory
   and to minimise writes.  */


typedef struct
{
    flashheap_size_t size;
} flashheap_packet_t;


typedef void
(*flashheap_callback_t)(flashheap_t heap, flashheap_addr_t addr,
                        flashheap_packet_t *packet, void *arg);


static flashheap_dev_t heap_data;

bool
flashheap_free (flashheap_t heap, void *ptr)
{
    flashheap_packet_t packet;
    flashheap_packet_t prev_packet;
    flashheap_packet_t next_packet;
    flashheap_addr_t prev_offset;
    flashheap_addr_t next_offset;
    flashheap_addr_t offset;
    flashheap_addr_t desired;

    desired = (flashheap_addr_t)ptr;
    prev_offset = 0;
    offset = heap->offset;
    prev_packet.size = 0;

    /* Linear search through heap looking for desired packet.  */
    while (offset < heap->offset + heap->size)
    {
        if (heap->read (heap->dev, offset, &packet,
                        sizeof (packet)) != sizeof (packet))
            return 0;

        if (offset == desired)
            break;
        
        prev_offset = offset;
        prev_packet = packet;
        offset += abs (packet.size) + sizeof (packet);
    }

    /* Couldn't find packet.  */
    if (offset != desired)
        return 0;

    if (packet.size < 0)
        return 0;
    packet.size = -packet.size;

    next_offset = offset + abs (packet.size) + sizeof (packet);
    if (heap->read (heap->dev, next_offset, &next_packet,
                    sizeof (next_packet)) != sizeof (next_packet))
        return 0;
    
    if (prev_packet.size < 0 && next_packet.size < 0)
    {
        /* Coalesce previous, current, and next packets.  */
        packet.size += prev_packet.size + next_packet.size
            - 2 * sizeof (packet);
        offset = prev_offset;
    }
    else if (next_packet.size < 0)
    {
        /* Coalesce current and next packets.  */
        packet.size += next_packet.size - sizeof (packet);
    }
    else if (prev_packet.size < 0)
    {
        /* Coalesce current and prev packets.  */
        packet.size += prev_packet.size - sizeof (packet);
        offset = prev_offset;
    }

    if (heap->write (heap->dev, offset, &packet,
                     sizeof (packet)) != sizeof (packet))
        return 0;

    return 1;
}


void *
flashheap_alloc (flashheap_t heap, flashheap_size_t size)
{
    flashheap_packet_t packet;
    flashheap_addr_t offset;

    offset = heap->offset;

    while (offset < heap->offset + heap->size)
    {
        if (heap->read (heap->dev, offset, &packet,
                        sizeof (packet)) != sizeof (packet))
            return 0;
        
        if (packet.size < 0 && size <= -packet.size)
        {
            /* Have a free packet so need to split.  */
            if (size != -packet.size)
            {
                flashheap_packet_t new_packet;
                flashheap_addr_t new_offset;

                new_offset = offset + sizeof (packet) + size;
                new_packet.size = -(-packet.size - size - sizeof (packet));

                if (heap->write (heap->dev, new_offset, &new_packet,
                                 sizeof (new_packet)) != sizeof (new_packet))
                    return 0;
            }

            packet.size = size;
            if (heap->write (heap->dev, offset, &packet,
                             sizeof (packet)) != sizeof (packet))
                return 0;            

            heap->last = offset;
            return (void *)offset;
        }
        /* Skip to start of next packet.  */
        offset += abs (packet.size) + sizeof (packet);
    }

    /* No free packet.  */
    return 0;
}


/* Iterate over all packets.  */
static bool
flashheap_walk (flashheap_t heap, flashheap_callback_t callback, void *arg)
{
    flashheap_packet_t packet;
    flashheap_addr_t offset;

    for (offset = heap->offset; offset < heap->offset + heap->size;
         offset += abs (packet.size) + sizeof (packet))
    {
        if (heap->read (heap->dev, offset, &packet,
                        sizeof (packet)) != sizeof (packet))
            return 0;

        callback (heap, offset, &packet, arg);
    }
    return 1;
}


static void
flashheap_stats_helper (flashheap_t heap, flashheap_addr_t addr,
                        flashheap_packet_t *ppacket, 
                        void *arg)
{
    flashheap_stats_t *pstats = arg;
    
    if (ppacket->size >= 0)
    {
        pstats->alloc_packets++;
        pstats->alloc_bytes += ppacket->size;
    }
    else
    {
        pstats->free_packets++;
        pstats->free_bytes -= ppacket->size;
    }
}


void
flashheap_stats (flashheap_t heap, flashheap_stats_t *pstats)
{
    pstats->alloc_packets = 0;
    pstats->free_packets = 0;
    pstats->alloc_bytes = 0;
    pstats->free_bytes = 0;

    flashheap_walk (heap, flashheap_stats_helper, pstats);
}


flashheap_t
flashheap_init (flashheap_addr_t offset, flashheap_size_t size,
                void *dev, flashheap_read_t read,
                flashheap_write_t write)
{
    flashheap_packet_t packet;
    flashheap_t heap;

    /* Note offset cannot be zero.  */

    /* Create one large empty packet.  */
    packet.size = -(size - sizeof (packet));

    heap = &heap_data;
    heap->dev = dev;
    heap->read = read;
    heap->write = write;
    heap->offset = offset;
    heap->size = size;

    if (heap->write (dev, offset, &packet,
                     sizeof (packet)) != sizeof (packet))
        return 0;

    return heap;
}

/** @file   flashheap.c
    @author Michael Hayes
    @date   23 February 2009
    @brief  Routines to implement a heap in a dataflash memory
            with wear-levelling.
*/

#include <flashheap.h>
#include <stdlib.h>

#include <stdio.h>

/* The goals of this code is to implement a heap within flash memory
   and to minimise writes.  Some flash devices have a more robust first block
   but we don't assume this.
*/


typedef struct
{
    flashheap_size_t size;
} flashheap_packet_t;


/* Return true if desire to terminate walk.  */
typedef bool
(*flashheap_callback_t)(flashheap_addr_t addr,
                        flashheap_packet_t *packet, void *arg);


static flashheap_dev_t heap_data;


static bool
flashheap_packet_read (flashheap_t heap, flashheap_addr_t addr, 
                       flashheap_packet_t *ppacket)
{
    iovec_t iov;

    iov.data = ppacket;
    iov.len = sizeof (*ppacket);

    return heap->readv (heap->dev, addr, &iov, 1) == sizeof (*ppacket);
}


static bool
flashheap_packet_write (flashheap_t heap, flashheap_addr_t addr, 
                        flashheap_packet_t *ppacket)
{
    iovec_t iov;

    iov.data = ppacket;
    iov.len = sizeof (*ppacket);

    return heap->writev (heap->dev, addr, &iov, 1) == sizeof (*ppacket);
}


bool
flashheap_free (flashheap_t heap, void *ptr)
{
    flashheap_packet_t packet;
    flashheap_packet_t prev_packet;
    flashheap_packet_t next_packet;
    flashheap_addr_t prev_addr;
    flashheap_addr_t next_addr;
    flashheap_addr_t addr;
    flashheap_addr_t desired;

    if (!ptr)
        return 0;

    desired = (flashheap_addr_t)ptr - sizeof (packet);
    prev_addr = 0;
    addr = heap->offset;
    prev_packet.size = 0;

    /* Linear search through heap looking for desired packet.  */
    while (addr < heap->offset + heap->size)
    {
        if (!flashheap_packet_read (heap, addr, &packet))
            return 0;

        if (addr == desired)
            break;
        
        prev_addr = addr;
        prev_packet = packet;
        addr += abs (packet.size) + sizeof (packet);
    }

    /* Couldn't find packet.  */
    if (addr != desired)
        return 0;

    /* Can't free a packet twice.  */
    if (packet.size < 0)
        return 0;

    packet.size = -packet.size;

    next_addr = addr + abs (packet.size) + sizeof (packet);
    if (!flashheap_packet_read (heap, next_addr, &next_packet))
        return 0;
    
    if (prev_packet.size < 0 && next_packet.size < 0)
    {
        /* Coalesce previous, current, and next packets.  */
        packet.size += prev_packet.size + next_packet.size
            - 2 * sizeof (packet);
        addr = prev_addr;
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
        addr = prev_addr;
    }

    if (!flashheap_packet_write (heap, addr, &packet))
        return 0;

    return 1;
}


/** This allocates a packet and writes data at the same time.  If
    iov[0].data is NULL then only a packet is allocated.  */
void *
flashheap_writev (flashheap_t heap, iovec_t *iov, iovec_count_t iov_count)
{
    flashheap_packet_t packet;
    flashheap_addr_t addr;
    int size;
    iovec_t iov2[4];
    unsigned int i;

    addr = heap->offset;

    /* Determine total number of data bytes to write.  */
    size = 0;
    for (i = 0; i < iov_count; i++)
        size += iov[i].len;

    /* What about allocating a zero sized packet?  malloc either
       returns NULL or a valid pointer that can be passed to free.
       Let's return NULL.  */
    if (!size)
        return 0;

    /* How do we do wear levelling?  We need to keep track
       of where we got to but we lose this info on reset.  */
    while (addr < heap->offset + heap->size)
    {
        if (!flashheap_packet_read (heap, addr, &packet))
            return 0;
        
        if (packet.size < 0 && size <= -packet.size)
        {
            /* Have found a free packet.  */
            if (size != -packet.size)
            {
                flashheap_packet_t new_packet;
                flashheap_addr_t new_addr;

                /* The packet is bigger than required so create
                   a new empty packet.  */
                new_addr = addr + sizeof (packet) + size;
                new_packet.size = -(-packet.size - size - sizeof (packet));

                if (!flashheap_packet_write (heap, new_addr, &new_packet))
                    return 0;
            }
            
            packet.size = size;
            
            iov2[0].data = &packet;
            iov2[0].len = sizeof (packet);
            if (iov[0].data)
            {
                for (i = 0; i < iov_count && i < ARRAY_SIZE (iov2) - 1; i++)
                    iov2[i + 1] = iov[i];

                /*  Write the packet header and first lot of data.  */
                heap->writev (heap->dev, addr, iov2, i + 1);

                /* Write other lots of data if required.  */
                if (iov_count >= ARRAY_SIZE (iov2) - 1)
                    heap->writev (heap->dev, addr, iov + i - 1,
                                  iov_count - i + 1);
            }
            else
            {
                /* Just write the packet header.  */
                heap->writev (heap->dev, addr, iov2, 1);
            }

            return (void *)addr + sizeof (packet);
        }
        /* Skip to start of next packet.  */
        addr += abs (packet.size) + sizeof (packet);
    }

    /* No free packet.  */
    return 0;
}


flashheap_size_t
flashheap_readv (flashheap_t heap, void *ptr, iovec_t *iov,
                 iovec_count_t iov_count)
{
    flashheap_addr_t addr;

    addr = (flashheap_addr_t)ptr;

    /* FIXME, check that have valid pointer.  */

    return heap->readv (heap->dev, addr, iov, iov_count);
}


/* Iterate over packets starting at addr.  Stop if callback returns true.  */
static bool
flashheap_walk (flashheap_t heap, flashheap_addr_t addr, 
                flashheap_callback_t callback, void *arg)
{
    flashheap_packet_t packet;

    for (; addr < heap->offset + heap->size;
         addr += abs (packet.size) + sizeof (flashheap_packet_t))
    {
        if (!flashheap_packet_read (heap, addr, &packet))
            return 0;

        if (callback (addr, &packet, arg))
            return 1;
    }
    return 0;
}


flashheap_size_t
flashheap_size_get (flashheap_t heap, void *ptr)
{
    flashheap_packet_t packet;
    flashheap_addr_t addr;

    addr = (flashheap_addr_t) ptr - sizeof (packet);
    
    if (!flashheap_packet_read (heap, addr, &packet))
        return 0;
    
    if (packet.size < 0)
        return 0;
    
    return packet.size;
}


static bool
flashheap_stats_helper (flashheap_addr_t addr __UNUSED__,
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
    return 0;
}


void
flashheap_stats (flashheap_t heap, flashheap_stats_t *pstats)
{
    pstats->alloc_packets = 0;
    pstats->free_packets = 0;
    pstats->alloc_bytes = 0;
    pstats->free_bytes = 0;

    flashheap_walk (heap, heap->offset, flashheap_stats_helper, pstats);
}


static bool
flashheap_debug_helper (flashheap_addr_t addr,
                        flashheap_packet_t *ppacket, 
                        void *arg __UNUSED__)
{
    printf ("%d: %d\n", (int)addr, (int)ppacket->size);
    return 0;
}


void
flashheap_debug (flashheap_t heap)
{
    flashheap_walk (heap, heap->offset, flashheap_debug_helper, NULL);
}


bool
flashheap_erase (flashheap_t heap)
{
    flashheap_packet_t packet;
    
    /* For wear levelling we could create two empty packets with the
       position of the second (starting) packet moving circularly in
       increments throughout the memory.  */

    /* Create one large empty packet.  */
    packet.size = -(heap->size - sizeof (packet));

    return flashheap_packet_write (heap, heap->offset, &packet);
}


void *
flashheap_alloc (flashheap_t heap, flashheap_size_t size)
{
    iovec_t iov;

    iov.data = NULL;
    iov.len = size;
    return flashheap_writev (heap, &iov, 1);
}


flashheap_t
flashheap_init (flashheap_addr_t offset, flashheap_size_t size,
                void *dev, flashheap_readv_t readv,
                flashheap_writev_t writev)
{
    flashheap_t heap;

    /* Note offset cannot be zero.  */

    heap = &heap_data;
    heap->dev = dev;
    heap->readv = readv;
    heap->writev = writev;
    heap->offset = offset;
    heap->size = size;

    return heap;
}

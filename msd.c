#include "msd.h"
#include <string.h>

/* Most mass storage devices are block-oriented.  The code here is
   designed to present a common interface to sdcard and other flash
   memory devices.  It supports partial block reads and partial block
   writes.

   For a partial block read we read a block into a temporary buffer
   then copy the desired part of the buffer to the user.  This buffer
   can also be used as a read cache.

   For a partial block write we read the block into a temporary buffer
   overwrite the buffer with the user's data then write the temporary
   buffer (assuming that the write does an erase first).  Some flash
   devices such as dataflash can do this using internal buffers but
   sdcards do not.  Unfortunately, devices such as microsd/microsdhc
   et al have pages of 32 or 64 blocks, each of 512 bytes.  This is
   the minimum size that can be erased so we need a really big buffer.

   An alternative approach is to put the onus on the sdcard driver to
   manage page modification.  It can do this by reserving a spare page
   (say at the end), erasing the spare page, then copying the page
   containing the block that needs to be modified to the spare page,
   erasing the page that needs to be modified, writing the new block contents,
   then copying the other block contents from the spare page.  Phew!
*/


#ifndef MSD_CACHE_SIZE
#define MSD_CACHE_SIZE 512
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct msd_cache_struct
{
    uint8_t data[MSD_CACHE_SIZE];
    /* This is the start address of a block.  */
    msd_addr_t addr;
    msd_t *msd;
} msd_cache_t;

static msd_cache_t msd_cache;


static msd_size_t 
msd_cache_fill (msd_t *msd, msd_addr_t addr)
{
    msd_size_t bytes;

    /* Need to flush cache if dirty.  */

    if (msd_cache.msd == msd && msd_cache.addr == addr)
        return MSD_CACHE_SIZE;

    bytes = msd->read (msd->handle, addr, msd_cache.data,
                       MSD_CACHE_SIZE);
    
    msd_cache.msd = msd;
    msd_cache.addr = addr;
    return bytes;
}


static msd_size_t 
msd_cache_flush (msd_t *msd, msd_addr_t addr)
{
    msd_size_t bytes;

    /* This assumes that the write routine does any erasing if
       necessary and that MSD_CACHE_SIZE is a multiple of the page
       size.  */
    bytes = msd->write (msd_cache.msd, addr, msd_cache.data,
                        MSD_CACHE_SIZE);

    /* Clear dirty bit.  */

    return bytes;
}


msd_size_t 
msd_read (msd_t *msd, msd_addr_t addr, void *buffer, msd_size_t size)
{
    msd_addr_t block;
    msd_size_t offset;
    msd_size_t total;
    msd_size_t bytes;
    uint8_t *dst = buffer;

    block = addr / MSD_CACHE_SIZE;
    offset = addr - block * MSD_CACHE_SIZE;
    addr = addr - offset;
    total = 0;

    while (size)
    {
        /* If bytes == MSD_CACHE_SIZE and bytes < size then could read
           directly to user's buffer since the cache will be
           overwriten on next read.  */

        bytes = msd_cache_fill (msd, addr);
        /* Perhaps should return error.  */
        if (bytes != MSD_CACHE_SIZE)
            return total;

        bytes = MIN (bytes - offset, size);
        memcpy (dst, msd_cache.data + offset, bytes);

        size -= bytes;
        addr += bytes;
        dst += bytes;
        total += bytes;
        offset = 0;
    }

    return total;
}


msd_size_t
msd_write (msd_t *msd, msd_addr_t addr, const void *buffer, msd_size_t size)
{
    msd_addr_t block;
    msd_size_t offset;
    msd_size_t total;
    msd_size_t bytes;
    const uint8_t *src = buffer;

    block = addr / MSD_CACHE_SIZE;
    offset = addr - block * MSD_CACHE_SIZE;
    addr = addr - offset;
    total = 0;

    while (size)
    {
        if (offset != 0 || size < MSD_CACHE_SIZE)
        {
            /* Have a partial write so need to perform
               read-modify-write.  */

            bytes = msd_cache_fill (msd, addr);
            /* Perhaps should return error.  */
            if (bytes != MSD_CACHE_SIZE)
                return total;
        }
        else
        {
            /* Need to flush cache if dirty.  */
            msd_cache.msd = msd;
            msd_cache.addr = addr;

            bytes = MSD_CACHE_SIZE;
        }

        bytes = MIN (bytes - offset, size);
        memcpy (msd_cache.data + offset, src, bytes);

        /* Implement write-through policy for now to ensure that data
           hits storage.  This is inefficient for many small
           writes and for large page sizes.  */

        bytes = msd_cache_flush (msd, addr);
        /* Perhaps should return error.  */
        if (bytes != MSD_CACHE_SIZE)
            return total;

        size -= bytes;
        addr += bytes;
        src += bytes;
        total += bytes;
        offset = 0;
    }

    return total;
}


msd_status_t
msd_status_get (msd_t *msd)
{
    return msd->status_get (msd->handle);
}


void
msd_shutdown (msd_t *msd)
{
    if (msd->shutdown)
        msd->shutdown (msd->handle);
}

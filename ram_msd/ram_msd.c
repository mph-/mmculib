#include "config.h"
#include "msd.h"
#include <string.h>

/* This only supports a single instance, however, it would 
   be trivial to use malloc to allow multiple instances.  */

#ifndef RAM_MSD_BYTES
#error RAM_MSD_BYTES undefined in config.h
#endif

#define RAM_MSD_BLOCK_BYTES 512
#define RAM_MSD_BLOCKS (RAM_MSD_BYTES / RAM_MSD_BLOCK_SIZE)

static uint8_t mem[RAM_MSD_BYTES];


static msd_size_t
ram_msd_read (void *dev __unused__, msd_addr_t addr,
              void *buffer, msd_size_t size)
{
    if (addr + size > RAM_MSD_BYTES)
        return 0;

    memcpy (buffer, &mem[addr], size);

    return size;
}


static msd_size_t
ram_msd_write (void *dev __unused__, msd_addr_t addr,
               const void *buffer, msd_size_t size)
{
    if (addr + size > RAM_MSD_BYTES)
        return 0;

    memcpy (&mem[addr], buffer, size);
    
    return size;
}


static msd_status_t
ram_msd_status_get (void *dev __unused__)
{
    return MSD_STATUS_READY;
}


static const msd_ops_t ram_msd_ops =
{
    .read = ram_msd_read,
    .write = ram_msd_write,
    .status_get = ram_msd_status_get
};


static msd_t ram_msd =
{
    .handle = 0,
    .ops = &ram_msd_ops,
    .media_bytes = RAM_MSD_BYTES,
    .block_bytes = RAM_MSD_BLOCK_BYTES,
    .flags = {.removable = 0, .volatile1 = 1, .reserved = 0},
    .name = "Ramdisk"
};


msd_t *
ram_msd_init (void)
{
    return &ram_msd;
}

/** @file   fat_io.c
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem I/O operations.
*/


/*  Note, FAT is horrendous in terms of efficiency.  Here we implement
    a simple write-back cache for a single sector.  This helps to cache the
    FAT, fsinfo, and directory entries.  Data sectors are not cached.   */


#include "fat_io.h"


uint16_t
fat_io_read (fat_t *fat, fat_sector_t sector,
             uint16_t offset, void *buffer, uint16_t size)
{
    return fat->dev_read (fat->dev, 
                          sector * fat->bytes_per_sector + offset, 
                          buffer, size);
}


uint16_t
fat_io_write (fat_t *fat, fat_sector_t sector,
               uint16_t offset, const void *buffer, uint16_t size)
{
    return fat->dev_write (fat->dev,
                           sector * fat->bytes_per_sector + offset, 
                           buffer, size);
}


void
fat_io_cache_flush (fat_t *fat)
{
    if (fat->cache.dirty)
    {
        fat_io_write (fat, fat->cache.sector, 0, fat->cache.buffer,
                       fat->bytes_per_sector);
        fat->cache.dirty = 0;
    }
}


uint8_t *
fat_io_cache_read (fat_t *fat, fat_sector_t sector)
{
    if (sector == fat->cache.sector)
        return fat->cache.buffer;

    fat_io_cache_flush (fat);

    fat->cache.sector = sector;
    if (fat_io_read (fat, fat->cache.sector, 0, 
                     fat->cache.buffer, fat->bytes_per_sector)
        != fat->bytes_per_sector)
    {
        TRACE_ERROR (FAT, "FAT:Cannot read sector = %ld\n", sector);
        return 0;
    }
    
    return fat->cache.buffer;
}


uint16_t
fat_io_cache_write (fat_t *fat, fat_sector_t sector)
{
    fat->cache.sector = sector;
    fat->cache.dirty = 1;
    /* Don't write through to device; need to call fat_io_cache_flush
       when finished.  */
    return fat->bytes_per_sector;
}


static void
fat_io_cache_init (fat_t *fat)
{
    fat->cache.sector = -1;
    fat->cache.dirty = 0;
}


void
fat_io_init (fat_t *fat, void * dev, fat_dev_read_t dev_read, fat_dev_write_t dev_write)
{
    fat->dev = dev;
    fat->dev_read = dev_read;
    fat->dev_write = dev_write;

    fat_io_cache_init (fat);
}

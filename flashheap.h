/** @file flashheap.h
 *  @author Michael Hayes
 *  @date 23 February 2009
 * 
 *  @brief Routines to implement a heap in a dataflash memory.
 */
 
#ifndef FLASHHEAP_H
#define FLASHHEAP_H

#include "config.h"

typedef int32_t flashheap_size_t;
typedef int32_t flashheap_addr_t;

typedef flashheap_size_t
(*flashheap_read_t)(void *dev, flashheap_addr_t addr,
                    void *buffer, flashheap_size_t len);


typedef flashheap_size_t 
(*flashheap_write_t)(void *dev, flashheap_addr_t addr,
                     const void *buffer, flashheap_size_t len);

typedef struct
{
    flashheap_addr_t offset;
    flashheap_size_t size;
    flashheap_addr_t last;
    void *dev;
    flashheap_read_t read;
    flashheap_write_t write;
} flashheap_dev_t;


typedef struct
{
    flashheap_size_t alloc_bytes;
    flashheap_size_t free_bytes;
    flashheap_size_t alloc_packets;
    flashheap_size_t free_packets;
} flashheap_stats_t;


typedef flashheap_dev_t *flashheap_t;


extern bool
flashheap_free (flashheap_t heap, void *ptr);


extern void *
flashheap_alloc (flashheap_t heap, flashheap_size_t size);


extern void *
flashheap_alloc_first (flashheap_t heap);


extern void *
flashheap_alloc_next (flashheap_t heap, void *ptr);


extern flashheap_size_t
flashheap_alloc_size (flashheap_t heap, void *ptr);

extern void
flashheap_stats (flashheap_t heap, flashheap_stats_t *pstats);


extern bool
flashheap_erase (flashheap_t heap);


extern flashheap_t
flashheap_init (flashheap_addr_t offset,
                flashheap_size_t size,
                void *dev, flashheap_read_t read,
                flashheap_write_t write);

#endif

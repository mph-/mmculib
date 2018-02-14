/** @file   flashheap.h
    @author Michael Hayes
    @date   23 February 2009
    @brief  Routines to implement a heap in a dataflash memory
            with wear-levelling.
*/
 
#ifndef FLASHHEAP_H
#define FLASHHEAP_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "iovec.h"

typedef int32_t flashheap_size_t;
typedef int32_t flashheap_addr_t;

typedef flashheap_size_t
(*flashheap_readv_t)(void *dev, flashheap_addr_t addr,
                     iovec_t *iov, iovec_count_t iovcount);


typedef flashheap_size_t 
(*flashheap_writev_t)(void *dev, flashheap_addr_t addr,
                      iovec_t *iov, iovec_count_t iovcount);

typedef struct
{
    flashheap_addr_t offset;    /* Address of first byte of heap.  */
    flashheap_size_t size;      /* Number of bytes in heap.  */
    void *dev;                  /* Handle for readv/writev routines.  */
    flashheap_readv_t readv;
    flashheap_writev_t writev;
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


extern flashheap_size_t
flashheap_size_get (flashheap_t heap, void *ptr);


extern void
flashheap_stats (flashheap_t heap, flashheap_stats_t *pstats);


extern bool
flashheap_erase (flashheap_t heap);


/** This is the principle function; it allocates a packet in flash,
    writes a vector of data, and returns a pointer to the first byte
    of the data or NULL for failure.  */
extern void *
flashheap_writev (flashheap_t heap, iovec_t *iov, iovec_count_t iov_count);


extern flashheap_size_t
flashheap_readv (flashheap_t heap, void *ptr, iovec_t *iov,
                 iovec_count_t iov_count);


extern void
flashheap_debug (flashheap_t heap);


extern flashheap_t
flashheap_init (flashheap_addr_t offset,
                flashheap_size_t size,
                void *dev, flashheap_readv_t readv,
                flashheap_writev_t writev);


#ifdef __cplusplus
}
#endif    
#endif


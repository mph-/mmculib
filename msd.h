#ifndef MSD_H
#define MSD_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

typedef uint16_t msd_size_t;
typedef uint64_t msd_addr_t;


#ifndef MSD_BLOCK_SIZE_MAX
#define MSD_BLOCK_SIZE_MAX 512
#endif

typedef enum
{
    MSD_STATUS_READY,
    MSD_STATUS_BUSY,
    MSD_STATUS_NODEVICE
} msd_status_t;


typedef struct
{
    unsigned int removable:1;
    unsigned int volatile1:1;
    unsigned int partial_read:1;
    unsigned int partial_write:1;
    unsigned int reserved:4;
} msd_flags_t;


typedef msd_addr_t
(*msd_probe_t)(void *handle);


typedef msd_size_t
(*msd_read_t)(void *handle, msd_addr_t addr, void *buffer, msd_size_t size);


typedef msd_size_t 
(*msd_write_t)(void *handle, msd_addr_t addr, const void *buffer, msd_size_t size);


typedef msd_status_t
(*msd_status_get_t)(void *handle);


typedef void
(*msd_shutdown_t)(void *shutdown);


typedef struct msd_ops_struct
{
    msd_probe_t probe;
    msd_read_t read;
    msd_write_t write;
    msd_status_get_t status_get;
    msd_shutdown_t shutdown;
} msd_ops_t;


typedef struct msd_struct
{
    void *handle;
    const msd_ops_t *ops;
    msd_addr_t media_bytes;
    msd_size_t block_bytes;
    uint32_t reads;
    uint32_t writes;
    uint16_t read_errors;
    uint16_t write_errors;
    const char *name;
    msd_flags_t flags;
} msd_t;


msd_addr_t msd_probe (msd_t *msd);

msd_size_t msd_read (msd_t *msd, msd_addr_t addr, void *buffer, msd_size_t size);

msd_size_t msd_write (msd_t *msd, msd_addr_t addr, const void *buffer, msd_size_t size);

msd_status_t msd_status_get (msd_t *msd);

void msd_shutdown (msd_t *msd);

static inline msd_addr_t msd_media_bytes_get (msd_t *msd)
{
    if (!msd)
        return 0;
    return msd->media_bytes;
}


#ifdef __cplusplus
}
#endif    
#endif


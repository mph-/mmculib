/** @file   fat.h
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT routines.
*/

#ifndef FAT_H_
#define FAT_H_

#include "config.h"
#include <unistd.h>
#include "fat_trace.h"

/* Size of a FAT sector.  */
#define FAT_SECTOR_SIZE 512

/* The maximum length of a file or directory name to use.  */
#ifndef FAT_NAME_LEN_USE
#define FAT_NAME_LEN_USE 32
#endif


#define CLUST_FREE      0               //!< Cluster 0 also means a free cluster
#define CLUST_FIRST     2               //!< First legal cluster number 
#define CLUST_RSRVD     0xfffffff6u     //!< Reserved cluster range 
#define CLUST_BAD       0xfffffff7u     //!< A cluster with a defect 
#define CLUST_EOFS      0xfffffff8u     //!< Start of eof cluster range 
#define CLUST_EOFE      0xffffffffu     //!< End of eof cluster range 


typedef uint16_t (*fat_dev_read_t) (void *dev, uint32_t addr,
                                    void *buffer, uint16_t size);

typedef uint16_t (*fat_dev_write_t) (void *dev, uint32_t addr, 
                                     const void *buffer, uint16_t size);


typedef uint32_t fat_sector_t;

struct fat_io_cache_struct
{
    uint32_t sector;                 //!< Cached sector number
    uint8_t buffer[FAT_SECTOR_SIZE]; //!< Cached sector data
    bool dirty;
};


typedef struct fat_io_cache_struct fat_io_cache_t;


typedef enum {FAT_FAT16, FAT_FAT32} fat_fs_type_t;


struct fat_struct
{                                       
    void *dev;                       //!< Device handle
    fat_dev_read_t dev_read;         //!< Device read function
    fat_dev_write_t dev_write;       //!< Device write function
    uint32_t first_sector;           //!< First sector
    uint32_t fsinfo_sector;          //!< File system info sector
    uint32_t first_fat_sector;       //!< First FAT sector
    uint32_t first_data_sector;      //!< First sector of the data area
    uint32_t num_fat_sectors;        //!< Number of sectors per FAT
    uint32_t first_dir_sector;       //!< First root directory sector
    uint32_t root_dir_cluster;       //!< First cluster of directory (FAT32)
    uint32_t num_clusters;           //!< Number of data clusters on partition
    uint32_t free_clusters;
    uint32_t prev_free_cluster;
    uint16_t root_dir_sectors;       //!< Number of sectors in root dir (FAT16)
    uint16_t bytes_per_sector;       //!< Number of bytes per sector
    uint16_t bytes_per_cluster;      //!< Number of bytes per cluster
    uint16_t sectors_per_cluster;
    fat_io_cache_t cache;
    fat_fs_type_t type;
    bool fsinfo_dirty;
};


typedef struct fat_struct fat_t;



static inline uint16_t le16_to_cpu (uint16_t val)
{
    return val;
}


static inline uint32_t le32_to_cpu (uint32_t val)
{
    return val;
}


static inline uint16_t cpu_to_le16 (uint16_t val)
{
    return val;
}


static inline uint32_t cpu_to_le32 (uint32_t val)
{
    return val;
}


static inline uint16_t le16_get (void *ptr)
{
    return le16_to_cpu (*(uint16_t *)ptr);
}


static inline uint32_t le32_get (void *ptr)
{
    return le32_to_cpu (*(uint32_t *)ptr);
}


static inline void le16_set (void *ptr, uint16_t val)
{
    *(uint16_t *)ptr = cpu_to_le16 (val);
}


static inline void le32_set (void *ptr, uint32_t val)
{
    *(uint32_t *)ptr = cpu_to_le32 (val);
}


fat_t *fat_init (void *dev, fat_dev_read_t dev_read, 
                    fat_dev_write_t dev_write);

uint32_t
fat_sector_calc (fat_t *fat, uint32_t cluster);


uint32_t
fat_chain_extend (fat_t *fat, uint32_t cluster_start, 
                  uint32_t num_clusters);

int
fat_dir_sector_count (fat_t *fat, uint32_t cluster);


uint16_t 
fat_chain_length (fat_t *fat, uint32_t cluster);


void
fat_cluster_chain_free (fat_t *fat, uint32_t cluster_start);


uint32_t 
fat_cluster_next (fat_t *fat, uint32_t cluster);


/* Return true if cluster is the last in the chain.  */
static inline bool
fat_cluster_last_p (uint32_t cluster)
{
    return cluster >= CLUST_EOFS;
}


bool
fat_check_p (fat_t *fat);


uint16_t
fat_sector_size (fat_t *fat);


uint16_t
fat_root_dir_cluster (fat_t *fat);

#endif


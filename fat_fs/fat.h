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
#include "fat_endian.h"

/* Size of a FAT sector.  */
#define FAT_SECTOR_SIZE 512


/* The maximum length of a file or directory name to use.  */
#ifndef FAT_NAME_LEN_USE
#define FAT_NAME_LEN_USE 32
#endif


typedef uint16_t (*fat_dev_read_t) (void *dev, uint32_t addr,
                                    void *buffer, uint16_t size);

typedef uint16_t (*fat_dev_write_t) (void *dev, uint32_t addr, 
                                     const void *buffer, uint16_t size);


struct fat_io_cache_struct
{
    /* Cached sector number.  */
    uint32_t sector;
    /* Cached sector data.  */
    uint8_t buffer[FAT_SECTOR_SIZE];
    bool dirty;
};


typedef struct fat_io_cache_struct fat_io_cache_t;


/** Supported FAT types.  */
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


uint32_t fat_sector_calc (fat_t *fat, uint32_t cluster);


uint32_t fat_chain_extend (fat_t *fat, uint32_t cluster_start, 
                           uint32_t num_clusters);


int fat_dir_sector_count (fat_t *fat, uint32_t cluster);


uint16_t fat_chain_length (fat_t *fat, uint32_t cluster);


void fat_cluster_chain_free (fat_t *fat, uint32_t cluster_start);


uint32_t fat_cluster_next (fat_t *fat, uint32_t cluster);


/* Return true if cluster is the last in the chain.  */
bool fat_cluster_last_p (uint32_t cluster);


bool fat_check_p (fat_t *fat);


uint16_t fat_sector_size (fat_t *fat);


uint16_t fat_root_dir_cluster (fat_t *fat);


fat_t *fat_init (void *dev, fat_dev_read_t dev_read, 
                 fat_dev_write_t dev_write);


#endif


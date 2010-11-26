/** @file   fat.c
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT filesystem routines for manipulating the FAT.
*/

#include "fat.h"
#include "fat_de.h"
#include "fat_io.h"
#include "fat_fsinfo.h"
#include "fat_cluster.h"
#include "fat_partition.h"


/*
   For a simplified description of FAT32 see 
   http://www.pjrc.com/tech/8051/ide/fat32.html
*/

#define CLUST_FIRST     2



/* Number of concurrent FAT file systems that can be supported.  */
#ifndef FAT_FS_NUM
#define FAT_FS_NUM 1
#endif


static fat_t fat_info[FAT_FS_NUM];
static uint8_t fat_num;



/** Return number of sectors for a directory.  */
int
fat_dir_sector_count (fat_t *fat, uint32_t cluster)
{
    if (fat->type == FAT_FAT16 && cluster == fat->root_dir_cluster)
        return fat->root_dir_sectors; 
    else
        return fat->sectors_per_cluster;
}


#if 0
static uint8_t
fat_checksum_calc (const char *filename)
{
    uint8_t checksum;
    int i;

    checksum = 0;
    for (i = 0; i < 12; i++)
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + *filename++;

    return checksum;
}
#endif


void
fat_dir_dump (fat_t *fat, uint32_t dir_cluster)
{
    fat_de_dir_dump (fat, dir_cluster);
}


void
fat_rootdir_dump (fat_t *fat)
{
    fat_dir_dump (fat, fat->root_dir_cluster);
}


bool
fat_check_p (fat_t *fat)
{
    /* Check for corrupt filesystem.  */
    return fat->bytes_per_cluster != 0
        && fat->bytes_per_sector != 0;
}


uint16_t
fat_sector_size (fat_t *fat)
{
    return fat->bytes_per_sector;
}


uint16_t
fat_root_dir_cluster (fat_t *fat)
{
    return fat->root_dir_cluster;
}


/**
 * Init FAT
 * 
 * Initialise things by reading partition information.
 * 
 */
fat_t *
fat_init (void *dev, fat_dev_read_t dev_read, fat_dev_write_t dev_write)
{
    fat_t *fat;

    fat = &fat_info[fat_num++];
    if (fat_num > FAT_FS_NUM)
        return NULL;

    fat->dev = dev;
    fat->dev_read = dev_read;
    fat->dev_write = dev_write;

    fat_io_cache_init (fat);

    TRACE_INFO (FAT, "FAT:Init\n");

    if (!fat_partition_read (fat))
        return 0;

    if (!fat_fsinfo_read (fat))
        return 0;

    TRACE_INFO (FAT, "FAT:Data sectors = %ld\n", data_sectors);
    TRACE_INFO (FAT, "FAT:Clusters = %ld\n", fat->num_clusters);
    TRACE_INFO (FAT, "FAT:Bytes/sector = %u\n",
                (unsigned int)fat->bytes_per_sector);
    TRACE_INFO (FAT, "FAT:Sectors/cluster = %u\n",
                (unsigned int)fat->sectors_per_cluster);
    TRACE_INFO (FAT, "FAT:First sector = %u\n",
                (unsigned int)fat->first_sector);
    TRACE_INFO (FAT, "FAT:FirstFATSector = %u\n", 
                (unsigned int)fat->first_fat_sector);
    TRACE_INFO (FAT, "FAT:FirstDataSector = %u\n",
                (unsigned int)fat->first_data_sector);
    TRACE_INFO (FAT, "FAT:FirstDirSector = %u\n", 
                (unsigned int)fat->first_dir_sector);
    TRACE_INFO (FAT, "FAT:RootDirCluster = %u\n",
                (unsigned int)fat->root_dir_cluster);

    return fat;
}

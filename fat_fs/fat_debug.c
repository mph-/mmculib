/** @file   fat_debug.c
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT filesystem debugging routines.
*/

#include "fat.h"
#include "fat_de.h"
#include "fat_trace.h"
#include "fat_file.h"
#include <sys/fcntl.h>


void
fat_debug_dir_dump (fat_t *fat, uint32_t dir_cluster)
{
    fat_de_dir_dump (fat, dir_cluster);
}


void
fat_debug_rootdir_dump (fat_t *fat)
{
    fat_debug_dir_dump (fat, fat->root_dir_cluster);
}


void 
fat_debug_file (fat_t *fat, const char *filename)
{
    fat_file_t *file;

    file = fat_open (fat, filename, O_RDONLY);
    if (!file)
        return;

    fat_file_debug (file);
    fat_close (file);
}


void
fat_debug_partition (fat_t *fat)
{
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
}



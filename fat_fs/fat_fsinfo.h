/** @file   fat_fsinfo.h
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem routines for the file system info.
*/

#ifndef FAT_FSINFO_H
#define FAT_FSINFO_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "fat.h"

void
fat_fsinfo_free_clusters_set (fat_t *fat, int count);


void
fat_fsinfo_free_clusters_update (fat_t *fat, int count);


void
fat_fsinfo_prev_free_cluster_set (fat_t *fat, uint32_t cluster);


uint32_t
fat_fsinfo_prev_free_cluster_get (fat_t *fat);


uint32_t
fat_fsinfo_free_clusters_get (fat_t *fat);


bool
fat_fsinfo_read (fat_t *fat);


void
fat_fsinfo_write (fat_t *fat);



#ifdef __cplusplus
}
#endif    
#endif


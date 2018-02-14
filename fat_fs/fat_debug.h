/** @file   fat_debug.h
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT filesystem debugging routines.
*/

#ifndef FAT_DEBUG_H
#define FAT_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "fat.h"

void fat_debug_dir_dump (fat_t *fat, uint32_t dir_cluster);


void fat_debug_rootdir_dump (fat_t *fat);


void fat_debug_file (fat_t *fat, const char *filename);


void fat_debug_partition (fat_t *fat);


#ifdef __cplusplus
}
#endif    
#endif



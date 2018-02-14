/** @file   fat_partition.h
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem routines for reading the partition record.
*/


#ifndef FAT_PARTITION_H
#define FAT_PARTITION_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "fat.h"


bool
fat_partition_read (fat_t *fat);



#ifdef __cplusplus
}
#endif    
#endif


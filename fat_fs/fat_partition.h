/** @file   fat_partition.h
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem routines for reading the partition record.
*/


#ifndef FAT_PARTITION_H
#define FAT_PARTITION_H

#include "fat.h"


bool
fat_partition_read (fat_t *fat);


#endif

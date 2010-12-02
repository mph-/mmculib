/** @file   fat_boot.h
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem routines for reading the boot record.
*/


#ifndef FAT_BOOT_H
#define FAT_BOOT_H

#include "fat.h"


bool
fat_boot_read (fat_t *fat);


#endif

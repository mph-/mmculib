/** @file   fat.h
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT32 filesystem routines.
*/

#ifndef FAT_H_
#define FAT_H_

#include "config.h"
#include <unistd.h>

typedef struct fat_fs_struct fat_fs_t;

typedef struct fat_struct fat_t;


typedef uint16_t (*fat_dev_read_t) (void *dev, uint32_t addr,
                                    void *buffer, uint16_t size);

typedef uint16_t (*fat_dev_write_t) (void *dev, uint32_t addr, 
                                        const void *buffer, uint16_t size);

fat_fs_t *fat_init (void *dev, fat_dev_read_t dev_read, 
                    fat_dev_write_t dev_write);

fat_t *fat_open (fat_fs_t *fat_fs, const char *name, int mode);

int fat_close (fat_t *fat);

ssize_t fat_read (fat_t *fat, void *buffer, size_t len);

ssize_t fat_write (fat_t *fat, const void *buffer, size_t len);

long fat_lseek (fat_t *fat, off_t offset, int whence);

int fat_unlink (fat_fs_t *fat_fs, const char *pathname);

#endif /*FAT_H_*/

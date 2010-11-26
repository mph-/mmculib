/** @file   fat_file.h
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem file operations.
*/

#ifndef FAT_FILE_H
#define FAT_FILE_H

#include "fat.h"


typedef struct fat_file_struct fat_file_t;


fat_file_t *fat_open (fat_t *fat, const char *pathname, int mode);

int fat_close (fat_file_t *file);

ssize_t fat_read (fat_file_t *file, void *buffer, size_t len);

ssize_t fat_write (fat_file_t *file, const void *buffer, size_t len);

off_t fat_lseek (fat_file_t *file, off_t offset, int whence);

int fat_unlink (fat_t *fat, const char *pathname);

int fat_mkdir (fat_t *fat, const char *pathname, mode_t mode);

#endif

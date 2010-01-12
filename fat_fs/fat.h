#ifndef FAT_H_
#define FAT_H_

#include "config.h"
#include "msd.h"
#include <unistd.h>

typedef struct fat_fs_struct fat_fs_t;

typedef struct fat_struct fat_t;

fat_fs_t *fat_init (msd_t *msd);

uint16_t fat_sector_bytes (fat_fs_t *fat);

uint32_t fat_size (fat_t *fat);

fat_t *fat_open (fat_fs_t *fat_fs, const char *name, int mode);

int fat_close (fat_t *fat);

ssize_t fat_read (fat_t *fat, char *buffer, size_t len);

ssize_t fat_write (fat_t *fat, const char *buffer, size_t len);

long fat_lseek (fat_t *fat, off_t offset, int dir);

int fat_chdir (fat_fs_t *fat_fs, const char *name);

int fat_unlink (const char *pathname);

#endif /*FAT_H_*/

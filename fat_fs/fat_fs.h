#ifndef FAT_FS_H
#define FAT_FS_H

#include "config.h"
#include "msd.h"
#include "sys.h"

bool fat_fs_init (msd_t *msd, sys_fs_t *fs);

#endif

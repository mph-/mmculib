#ifndef FAT_FS_H
#define FAT_FS_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "msd.h"
#include "sys.h"

bool fat_fs_init (msd_t *msd, sys_fs_t *fs);


#ifdef __cplusplus
}
#endif    
#endif


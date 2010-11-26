#include "fat_file.h"
#include "msd.h"
#include "sys.h"

/* This file is a wrapper for fat.c to keep fat.c more general and
   have fewer dependencies.  */


static const sys_devops_t fat_devops =
{
    .open = (void *)fat_open,
    .read = (void *)fat_read,
    .write = (void *)fat_write,
    .close = (void *)fat_close,
    .lseek = (void *)fat_lseek
};


static const sys_fsops_t fat_fsops =
{
    .unlink = (void *)fat_unlink,
    .rename = 0
};


static uint16_t
fat_fs_dev_read (void *arg, uint32_t addr, void *buffer, uint16_t size)
{
    msd_t *msd = arg;

    return msd_read (msd, addr, buffer, size);
}


static uint16_t
fat_fs_dev_write (void *arg, uint32_t addr, const void *buffer, uint16_t size)
{
    msd_t *msd = arg;

    return msd_write (msd, addr, buffer, size);
}


bool
fat_fs_init (msd_t *msd, sys_fs_t *fat_fs)
{
    void *arg;

    arg = fat_init (msd, fat_fs_dev_read, fat_fs_dev_write);
    if (!arg)
        return 0;

    fat_fs->devops = &fat_devops;
    fat_fs->fsops = &fat_fsops;
    fat_fs->private = arg;
    return 1;
}



#include "fat_file.h"
#include "fat_partition.h"
#include "fat_io.h"
#include "msd.h"
#include "sys.h"

/* Number of concurrent FAT file systems that can be supported.  */
#ifndef FAT_FS_NUM
#define FAT_FS_NUM 1
#endif


static fat_t fat_fs_info[FAT_FS_NUM];
static uint8_t fat_fs_num;


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
    fat_t *fat;

    if (fat_fs_num >= FAT_FS_NUM)
        return 0;

    fat = &fat_fs_info[fat_fs_num];

    fat_io_init (fat, msd, fat_fs_dev_read, fat_fs_dev_write);

    if (!fat_partition_read (fat))
        return 0;

    fat_fs_num++;

    fat_fs->devops = &fat_devops;
    fat_fs->fsops = &fat_fsops;
    fat_fs->private = fat;
    return 1;
}



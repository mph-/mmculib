#include "fat.h"
#include "msd.h"
#include "sys.h"

static sys_dev_t fat_dev =
{
    .open = (void *)fat_open,
    .read = (void *)fat_read,
    .write = (void *)fat_write,
    .close = (void *)fat_close,
    .lseek = (void *)fat_lseek
};


static sys_fs_t fat_fs =
{
    .name = "FAT",
    .dev = &fat_dev,
    .unlink = (void *)fat_unlink
};


bool fat_fs_init (msd_t *msd)
{
    void *arg;

    arg = fat_init (msd);
    if (!arg)
        return 0;
    sys_fs_register (&fat_fs, arg);
    return 1;
}

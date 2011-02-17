#include <stdio.h>
#include "fat_file.h"
#include "fat_debug.h"


static uint16_t
dev_read (void *arg, uint32_t addr, void *buffer, uint16_t size)
{
    FILE *fs = arg;

    fseek (fs, addr, SEEK_SET);

    return fread (buffer, 1, size, fs);
}


static uint16_t
dev_write (void *arg, uint32_t addr, const void *buffer, uint16_t size)
{
    FILE *fs = arg;

    fseek (fs, addr, SEEK_SET);

    return fwrite (buffer, 1, size, fs);
}


int main (int argc, char **argv)
{
    fat_t fat_info;
    fat_t *fat = &fat_info;
    FILE *fs;

    if (argc < 1)
        return 3;

    /* The file must be a formatted FAT file system.  */
    fs = fopen (argv[1], "r+");
    if (!fs)
        return 1;

    if (!fat_init (fat, fs, dev_read, dev_write))
        return 2;

    fat_debug_partition (fat);

    fat_test1 (fat, 50, 17);

    fat_debug_rootdir_dump (fat);

    fclose (fs);
    return 0;
}

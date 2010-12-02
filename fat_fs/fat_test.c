#include "fat_file.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>

int
fat_test1 (fat_t *fat, int num, int blocks)
{
    int i;

    for (i = 0; i < num; i++)
    {
        uint8_t buffer[512];
        char filename[32];
        fat_file_t *file;
        int j;

        sprintf (filename, "test%03d.tst", i);
        memset (buffer, i, 512);

        file = fat_open (fat, filename, O_CREAT | O_RDWR | O_TRUNC);
        if (!file)
            break;

        for (j = 0; j < blocks; j++)
            fat_write (file, buffer, 512);

        fat_close (file);
    }
    return i;
}

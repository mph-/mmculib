/** @file   fat_file.c
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem file operations.
*/

#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "fat_file.h"
#include "fat_de.h"
#include "fat_io.h"

struct fat_file_struct 
{
    fat_t *fat;
    int mode;         
    uint32_t file_offset;
    uint32_t file_size;  
    uint32_t file_alloc; 
    uint32_t start_cluster;
    uint32_t cluster;
    fat_dir_t dir;
};


static fat_file_t *
fat_create (fat_file_t *file, const char *pathname, fat_ff_t *ff)
{
    fat_t *fat;
    const char *filename;

    /* Check that directory is valid.  */
    /* TODO, should we create a directory?.  */
    if (!ff->parent_dir_cluster)
        return NULL;

    filename = pathname;
    while (strchr (filename, '/'))
        filename = strchr (filename, '/') + 1;

    /* TODO, what about a trailing slash?.  */

    fat = file->fat;

    file->file_offset = 0; 
    file->file_alloc = 0; 
    file->file_size = 0;
    file->start_cluster = 0;
    file->cluster = 0;

    /* Add file to directory.  */
    if (!fat_de_add (fat, &file->dir, filename, ff->parent_dir_cluster))
        return NULL;

    fat_io_cache_flush (fat);

    return file;
}


/* Search the filesystem for the directory entry for pathname, a file
   or directory.  */
static bool
fat_search (fat_t *fat, const char *pathname, fat_ff_t *ff)
{
    const char *p;
    char *q;
    char tmp[FAT_NAME_LEN_USE];

    if (pathname == NULL || !*pathname)
        return 0;

    p = pathname;

    ff->parent_dir_cluster = fat_root_dir_cluster (fat);
    
    while (*p)
    {
        /* Extract next part of path.  */
        q = tmp;
        while (*p && *p != '/')
            *q++ = *p++;
        *q = 0;

        /* Give up if find // within pathname.  */
        if (!*tmp)
            return 0;

        if (!fat_de_find (fat, ff->parent_dir_cluster, tmp, ff))
        {
            TRACE_INFO (FAT, "FAT:%s not found\n", tmp);

            /* If this should be a directory but it was not found then
               flag parent_dir_cluster as invalid.  */
            if (*p == '/')
                ff->parent_dir_cluster = 0;
            return 0;
        }

        if (*p == '/')
        {
            p++;

            if (!ff->isdir)
            {
                TRACE_ERROR (FAT, "FAT:%s not a dir\n", tmp);
                return 0;
            }

            if (*p)
                ff->parent_dir_cluster = ff->cluster;
        }
    }

    return 1;
}


static fat_file_t *
fat_find (fat_file_t *file, const char *pathname, fat_ff_t *ff)
{
    /* 
       foo/bar    file
       foo/bar/   dir
       foo/       dir in root dir
       foo        file or dir
       If remove trailing slash have 2 options
       foo/bar    file
       foo        file or dir.  */


    if (!fat_search (file->fat, pathname, ff))
        return NULL;

    TRACE_INFO (FAT, "FAT:Found %s\n", pathname);

    file->start_cluster = ff->cluster;
    file->cluster = file->start_cluster;
    file->file_offset = 0; 
    file->file_alloc = fat_chain_length (file->fat, file->start_cluster)
        * file->fat->bytes_per_cluster;
    file->file_size = ff->file_size;
    file->dir = ff->dir;
    return file;
}


/**
 * Open a file
 * 
 * @param name File name
 * @param mode Mode to open file
 * 
 * - O_BINARY Raw mode.
 * - O_TEXT End of line translation. 
 * - O_EXCL Open only if it does not exist. 
 * - O_RDONLY Read only. 
    
 Any of the write modes will (potentially) modify the file or directory entry

 * - O_CREAT Create file if it does not exist. 
 * - O_APPEND Always write at the end. 
 * - O_RDWR Read and write. 
 * - O_WRONLY Write only.

 * - O_TRUNC Truncate file if it exists. 
 *
 * @return File handle 
 */
fat_file_t *
fat_open (fat_t *fat, const char *pathname, int mode)
{
    fat_ff_t ff;
    fat_file_t *file;

    if (!fat_check_p (fat))
    {
        errno = EFAULT;
        return 0;
    }

    TRACE_INFO (FAT, "FAT:Open %s\n", pathname);

    if (!pathname || !*pathname)
        return 0;

    file = malloc (sizeof (*file));
    if (!file)
    {
        TRACE_ERROR (FAT, "FAT:Cannot alloc mem\n");
        return 0;
    }
    memset (file, 0, sizeof (*file));
    file->mode = mode;
    file->fat = fat;

    if (fat_find (file, pathname, &ff))
    {
        if (ff.isdir)
        {
            errno = EISDIR;
            return 0;
        }

        if ((mode & O_TRUNC) && (mode & O_RDWR || mode & O_WRONLY))
        {
            file->file_size = 0;
            file->file_alloc = 0;

            /* Remove all the previously allocated clusters.  */
            fat_cluster_chain_free (fat, file->start_cluster);
            file->start_cluster = 0;
            file->cluster = 0;

            fat_de_size_set (file->fat, &file->dir, file->file_size);
            fat_io_cache_flush (file->fat);
        }

        file->file_offset = 0;
        if (mode & O_APPEND)
            file->file_offset = file->file_size;        
        return file;
    }

    if (mode & O_CREAT)
    {
        if (fat_create (file, pathname, &ff))
        {
            if (mode & O_APPEND)
                file->file_offset = file->file_size;        
            return file;
        }
        TRACE_INFO (FAT, "FAT:%s not created\n", pathname);
    }
    else
    {
        TRACE_INFO (FAT, "FAT:%s not found\n", pathname);
    }

    free (file);
    return 0;
}


int
fat_unlink (fat_t *fat, const char *pathname)
{
    fat_ff_t ff;

    TRACE_INFO (FAT, "FAT:Unlink %s\n", pathname);

    /* Should figure out if the file is open and return EBUSY.  Would
       need to maintain a table of open files.  Let's assume that this
       is handled at a higher level.  */

    if (!fat_search (fat, pathname, &ff))
    {
        errno = ENOENT;
        return -1;
    }

    if (ff.isdir)
    {
        /* TODO, Need to scan the directory and check it is empty.  */
        /* For now just punt.  */
        errno = EISDIR;
        return -1;
    }

    fat_cluster_chain_free (fat, ff.cluster);

    fat_de_slot_delete (fat, &ff.dir, ff.parent_dir_cluster);

    TRACE_ERROR (FAT, "FAT:Unlink lost dir entry\n");
    return 0;
}


ssize_t
fat_write (fat_file_t *file, const void *buffer, size_t len)
{
    uint32_t sector;
    uint16_t ret;
    uint16_t nbytes;
    uint16_t bytes_left;
    uint16_t offset;
    uint16_t bytes_per_cluster;
    uint16_t bytes_per_sector;
    const uint8_t *data;
    bool newfile;

    TRACE_INFO (FAT, "FAT:Write %u\n", (unsigned int)len);

    if (! ((file->mode & O_RDWR) || (file->mode & O_WRONLY)))
    {
        errno = EINVAL;
        return -1;
    }

    /* To minimise writes to flash:
       1. allocate additional clusters (this will modify the FAT)
       2. update the fsinfo (number of free clusters, next free cluster)
       3. write the data (this might require traversal of FAT chain)
       4. update the directory entry (start cluster, file size,
          modification time).  */

    bytes_per_cluster = file->fat->bytes_per_cluster;
    bytes_per_sector = file->fat->bytes_per_sector;
    newfile = 0;

    if (file->file_alloc < len + file->file_size)
    {
        uint32_t cluster;
        uint32_t num_clusters;

        num_clusters = (len + file->file_size - file->file_alloc
                        + bytes_per_cluster - 1) / bytes_per_cluster;    
        
        cluster = fat_chain_extend (file->fat, file->cluster, num_clusters);
        file->file_alloc += num_clusters * bytes_per_cluster;

        if (!file->start_cluster)
        {
            file->start_cluster = cluster;
            newfile = 1;
        }
    }

    data = buffer;
    bytes_left = len;
    while (bytes_left)
    {
        /* If at cluster boundary find next cluster.  */
        if (file->file_offset % bytes_per_cluster == 0)
        {
            if (!file->cluster)
                file->cluster = file->start_cluster;
            else
            {
                file->cluster = fat_cluster_next (file->fat, file->cluster);
                if (fat_cluster_last_p (file->cluster))
                {
                    TRACE_ERROR (FAT, "Trying to write past allocated clusters\n");
                    break;
                }
            }
        }

        sector = fat_sector_calc (file->fat, file->cluster);

        sector += (file->file_offset % bytes_per_cluster) / bytes_per_sector;

        offset = file->file_offset % bytes_per_sector;

        /* Limit to remaining bytes in a sector.  */
        nbytes = bytes_left < bytes_per_sector - offset
            ? bytes_left : bytes_per_sector - offset;

        ret = fat_io_write (file->fat, sector, offset, data, nbytes);
        /* Give up if have write error.  */
        if (ret != nbytes)
            break;

        data += nbytes;
        file->file_offset += nbytes;
        file->file_size += nbytes;
        bytes_left -= nbytes;
    }

    /* Update directory entry.  */
    fat_de_size_set (file->fat, &file->dir, file->file_size);
    if (newfile)
        fat_de_cluster_set (file->fat, &file->dir, file->start_cluster);

    /* Should set modification time here.  */

    fat_io_cache_flush (file->fat);
    file->fat->fsinfo_dirty = 0;

    TRACE_INFO (FAT, "FAT:Write %u\n", (unsigned int)len - bytes_left);
    return len - bytes_left;
}


void 
fat_debug (fat_t *fat, const char *filename)
{
    fat_file_t *file;
    uint32_t cluster;

    file = fat_open (fat, filename, O_RDONLY);
    if (!file)
        return;

    fprintf (stderr, "Offset %lu\n", file->file_offset);
    fprintf (stderr, "Size %lu\n", file->file_size);
    fprintf (stderr, "Alloc %lu\n", file->file_alloc);
    fprintf (stderr, "DE sector %lu\n", file->dir.sector);
    fprintf (stderr, "DE offset %u\n", file->dir.offset);
    fprintf (stderr, "Clusters ");

    cluster = file->start_cluster;
    while (1)
    {
        fprintf (stderr, "%lu ", cluster);        
        if (!cluster)
            break;

        cluster = fat_cluster_next (file->fat, cluster);
        if (fat_cluster_last_p (cluster))
            break;
    }
    fprintf (stderr, "\n");
}


/**
 * Close a file.
 * 
 * @param fat File handle
 * @return Error code
 * 
 */
int
fat_close (fat_file_t *file)
{
    TRACE_INFO (FAT, "FAT:Close\n");

    if (file == NULL)
        return (uint32_t) -1;

    free (file);
    return 0;
}


/**
 * Read specific number of bytes from file
 * 
 * @param file File handle
 * @param buffer Buffer to store data
 * @param len Number of bytes to read
 * @return Number of bytes successfully read.
 */
ssize_t
fat_read (fat_file_t *file, void *buffer, size_t len)
{
    uint16_t nbytes;
    uint16_t ret;
    uint32_t sector;
    uint16_t bytes_left;
    uint16_t offset;
    uint8_t *data;

    TRACE_INFO (FAT, "FAT:Read %u\n", (unsigned int)len);
    
    /* Limit max read to size of file.  */
    if ((uint32_t)len > (file->file_size - file->file_offset))
        len = file->file_size - file->file_offset;
    
    data = buffer;
    bytes_left = len;
    while (bytes_left)
    {
        offset = file->file_offset % file->fat->bytes_per_sector;
        sector = fat_sector_calc (file->fat, file->cluster);

        /* Add local sector within a cluster .  */
        sector += (file->file_offset % file->fat->bytes_per_cluster) 
            / file->fat->bytes_per_sector;

        /* Limit to max one sector.  */
        nbytes = bytes_left < file->fat->bytes_per_sector
            ? bytes_left : file->fat->bytes_per_sector;

        /* Limit to remaining bytes in a sector.  */
        if (nbytes > (file->fat->bytes_per_sector - offset))
            nbytes = file->fat->bytes_per_sector - offset;

        /* Read the data; this does not affect the cache.  */
        ret = fat_io_read (file->fat, sector, offset, data, nbytes);
        /* Give up if have read error.  */
        if (ret != nbytes)
            break;

        data += nbytes;

        file->file_offset += nbytes;
        bytes_left -= nbytes;

        /* Cluster boundary.  */
        if (file->file_offset % file->fat->bytes_per_cluster == 0)
        {
            /* Need to move to next cluster in chain.  */
            file->cluster = fat_cluster_next (file->fat, file->cluster);

            /* If at end of chain there is no more data.  */
            if (fat_cluster_last_p (file->cluster))
                break;
        }
    }
    TRACE_INFO (FAT, "FAT:Read %u\n", (unsigned int)len - bytes_left);
    return len - bytes_left;
}


/**
 * Seek to specific position within file.
 * 
 * @param fat File handle
 * @param offset Number of bytes to seek
 * @param whence Position from where to start
 * @return New position within file
 */
off_t
fat_lseek (fat_file_t *file, off_t offset, int whence)
{
    off_t fpos = 0;
    unsigned int num;

    /* Setup position to seek from.  */
    switch (whence)
    {
    case SEEK_SET : fpos = offset;
        break;
    case SEEK_CUR : fpos = file->file_offset + offset;
        break;
    case SEEK_END : fpos = file->file_size + offset;
        break;
    }
    
    /* Apply limits.  */
    if (fpos < 0)
        fpos = 0;
    if ((uint32_t)fpos > file->file_size)
        fpos = file->file_size;

    /* Set the new position.  */
    file->file_offset = fpos;

    /* Calculate how many clusters from start cluster.  */
    num = fpos / file->fat->bytes_per_cluster;

    /* Set start cluster.  */
    file->cluster = file->start_cluster;

    /* Follow chain.  I wonder if it would be better to mark
       file->cluster being invalid and then fix it up when read or
       write is called?.  */
    while (num--)
    {
        uint32_t cluster_new;

        cluster_new = fat_cluster_next (file->fat, file->cluster);

        if (!fat_cluster_last_p (cluster_new))
            file->cluster = cluster_new;
        else
            break;
    }

    return fpos; 
}



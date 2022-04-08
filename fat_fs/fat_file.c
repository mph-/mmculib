/** @file   fat_file.c
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem file operations.
*/

#include <sys/fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "fat_partition.h"
#include "fat_cluster.h"
#include "fat_file.h"
#include "fat_de.h"
#include "fat_io.h"

struct fat_file_struct 
{
    fat_t *fat;
    int mode;         
    uint32_t offset;
    uint32_t size;  
    uint32_t alloc; 
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
    if (!ff->parent_dir_cluster)
        return NULL;

    filename = pathname;
    while (strchr (filename, '/'))
        filename = strchr (filename, '/') + 1;

    /* TODO, what about a trailing slash?.  */

    fat = file->fat;

    file->offset = 0; 
    file->alloc = 0; 
    file->size = 0;
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
bool
fat_search (fat_t *fat, const char *pathname, fat_ff_t *ff)
{
    const char *p;
    char *q;
    char tmp[FAT_NAME_LEN_USE];

    if (pathname == NULL || !*pathname)
        return 0;

    p = pathname;

    if (*p == '/')
        p++;

    ff->parent_dir_cluster = fat->root_dir_cluster;
    
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
    file->offset = 0; 
    file->alloc = fat_cluster_chain_length (file->fat, file->start_cluster)
        * file->fat->bytes_per_cluster;
    file->size = ff->size;
    file->dir = ff->dir;
    return file;
}


/**
 * Open a file
 * 
 * @param name File name
 * @param mode Mode to open file
 * 
 * - O_EXCL Open only if it does not exist (TODO). 
 * - O_RDONLY Read only. 
 * - O_CREAT Create file if it does not exist. 
 * - O_APPEND Always write at the end. 
 * - O_RDWR Read and write. 
 * - O_WRONLY Write only.
 * - O_TRUNC Truncate file if it exists. 
 *
 * @return File handle 
 * @note Any of the write modes may modify the file or directory entry
 */
fat_file_t *
fat_open (fat_t *fat, const char *pathname, int mode)
{
    fat_ff_t ff;
    fat_file_t *file;

    /* Check for corrupt filesystem.  */
    if (fat->bytes_per_cluster == 0
         || fat->bytes_per_sector == 0)
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
            file->size = 0;
            file->alloc = 0;

            /* Remove all the previously allocated clusters.  */
            fat_cluster_chain_free (fat, file->start_cluster);
            file->start_cluster = 0;
            file->cluster = 0;

            fat_de_size_set (file->fat, &file->dir, file->size);
            fat_io_cache_flush (file->fat);
        }

        file->offset = 0;
        if (mode & O_APPEND)
            file->offset = file->size;        
        return file;
    }

    if (mode & O_CREAT)
    {
        if (fat_create (file, pathname, &ff))
        {
            if (mode & O_APPEND)
                file->offset = file->size;        
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

    /* Should figure out if the file is open and return EBUSY.  This
       would need a table of open files.  Let's assume that this is
       handled at a higher level.  */

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


/**
 * Write specific number of bytes to file
 * 
 * @param file File handle
 * @param buffer Buffer to read from
 * @param len Number of bytes to read
 * @return Number of bytes successfully written.
 */
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

    TRACE_INFO (FAT, "FAT:Writing %u\n", (unsigned int)len);

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

    if (file->alloc < len + file->size)
    {
        uint32_t cluster;
        uint32_t num_clusters;

        num_clusters = (len + file->size - file->alloc
                        + bytes_per_cluster - 1) / bytes_per_cluster;    
        
        cluster = fat_cluster_chain_extend (file->fat, file->cluster, num_clusters);
        file->alloc += num_clusters * bytes_per_cluster;

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
        if (file->offset % bytes_per_cluster == 0)
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

        sector = fat_cluster_to_sector (file->fat, file->cluster);

        sector += (file->offset % bytes_per_cluster) / bytes_per_sector;

        offset = file->offset % bytes_per_sector;

        /* Limit to remaining bytes in a sector.  */
        nbytes = bytes_left < bytes_per_sector - offset
            ? bytes_left : bytes_per_sector - offset;

        ret = fat_io_write (file->fat, sector, offset, data, nbytes);
        /* Give up if have write error.  */
        if (ret != nbytes)
            break;

        data += nbytes;
        file->offset += nbytes;
        file->size += nbytes;
        bytes_left -= nbytes;
    }

    /* Update directory entry.  */
    fat_de_size_set (file->fat, &file->dir, file->size);
    if (newfile)
        fat_de_cluster_set (file->fat, &file->dir, file->start_cluster);

    /* Should set modification time here.  */

    fat_io_cache_flush (file->fat);
    file->fat->fsinfo_dirty = 0;

    TRACE_INFO (FAT, "FAT:Wrote %u\n", (unsigned int)len - bytes_left);
    return len - bytes_left;
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

    TRACE_INFO (FAT, "FAT:Reading %u\n", (unsigned int)len);
    
    /* Limit max read to size of file.  */
    if ((uint32_t)len > (file->size - file->offset))
        len = file->size - file->offset;
    
    data = buffer;
    bytes_left = len;
    while (bytes_left)
    {
        offset = file->offset % file->fat->bytes_per_sector;
        sector = fat_cluster_to_sector (file->fat, file->cluster);

        /* Add local sector within a cluster .  */
        sector += (file->offset % file->fat->bytes_per_cluster) 
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

        file->offset += nbytes;
        bytes_left -= nbytes;

        /* Cluster boundary.  */
        if (file->offset % file->fat->bytes_per_cluster == 0)
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
    case SEEK_CUR : fpos = file->offset + offset;
        break;
    case SEEK_END : fpos = file->size + offset;
        break;
    }
    
    /* Apply limits.  */
    if (fpos < 0)
        fpos = 0;
    if ((uint32_t)fpos > file->size)
        fpos = file->size;

    /* Set the new position.  */
    file->offset = fpos;

    /* Calculate how many clusters from start cluster.  */
    num = fpos / file->fat->bytes_per_cluster;

    /* Set start cluster.  */
    file->cluster = file->start_cluster;

    /* Follow chain.  I wonder if it would be better to mark
       file->cluster as invalid and then fix it up when read or
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


int
fat_mkdir (fat_t *fat, const char *pathname, mode_t mode)
{
    /* TODO.  */
    return -EACCES;
}


void
fat_file_debug (fat_file_t *file)
{
    TRACE_INFO (FAT, "Offset %lu\n", file->offset);
    TRACE_INFO (FAT, "Size %lu\n", file->size);
    TRACE_INFO (FAT, "Alloc %lu\n", file->alloc);
    TRACE_INFO (FAT, "DE sector %lu\n", file->dir.sector);
    TRACE_INFO (FAT, "DE offset %u\n", file->dir.offset);
    TRACE_INFO (FAT, "Clusters ");

    fat_cluster_chain_dump (file->fat, file->start_cluster);
}


/**
 * Register I/O functions for reading/writing filesystem
 * and if FAT file system found then initialise a new instance.
 * 
 * @param fat Pointer to FAT file system structure
 * @param dev Private argument for I/O routines
 * @param dev_read Function for reading
 * @param dev_write Function for writing
 * @return true if FAT file system found
 */
bool
fat_init (fat_t *fat, void *dev, fat_dev_read_t dev_read,
          fat_dev_write_t dev_write)
{
    fat_io_init (fat, dev, dev_read, dev_write);

    if (!fat_partition_read (fat))
        return 0;

    return 1;
}

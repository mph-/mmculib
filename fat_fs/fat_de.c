/** @file   fat_de.c
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem directory entry operations.
*/


#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include "fat_de.h"
#include "fat_cluster.h"
#include "fat_io.h"


/* Size of a winentry.  */
#define WIN_CHARS 13

#define SLOT_EMPTY      0x00      //!< Slot has never been used 
#define SLOT_E5         0x05      //!< The real value is 0xe5 
#define SLOT_DELETED    0xe5      //!< File in this slot deleted 


#define ATTR_NORMAL     0x00      //!< Normal file 
#define ATTR_READONLY   0x01      //!< File is readonly 
#define ATTR_HIDDEN     0x02      //!< File is hidden 
#define ATTR_SYSTEM     0x04      //!< File is a system file 
#define ATTR_VOLUME     0x08      //!< Entry is a volume label 
#define ATTR_LONG_FILENAME  0x0f  //!< Entry is a long filename
#define ATTR_DIRECTORY  0x10      //!< Entry is a directory name 
#define ATTR_ARCHIVE    0x20      //!< File is new or modified 

#define ATTR_WIN95      0x0f      //!< Attribute of LFN for Win95


#define LCASE_BASE      0x08      //!< Filename base in lower case 
#define LCASE_EXT       0x10      //!< Filename extension in lower case 

#define WIN_LAST        0x40      //!< Last sequence indicator
#define WIN_CNT         0x3f      //!< Sequence number mask


/**
 * This is the format of the contents of the deTime field in the direntry
 * structure.
 * We don't use bitfields because we don't know how compilers for
 * arbitrary machines will lay them out.
 * 
 */
#define DT_2SECONDS_MASK        0x1F    //!< seconds divided by 2 
#define DT_2SECONDS_SHIFT       0       //!< -
#define DT_MINUTES_MASK         0x7E0   //!< minutes 
#define DT_MINUTES_SHIFT        5       //!< -
#define DT_HOURS_MASK           0xF800  //!< hours 
#define DT_HOURS_SHIFT          11      //!< -


/**
 * Format of the contents of the deDate field in the direntry
 * structure.
 */
#define DD_DAY_MASK             0x1F    //!< day of month 
#define DD_DAY_SHIFT            0       //!< -
#define DD_MONTH_MASK           0x1E0   //!< month 
#define DD_MONTH_SHIFT          5       //!< -
#define DD_YEAR_MASK            0xFE00  //!< year - 1980 
#define DD_YEAR_SHIFT           9       //!< -



/** FAT directory entry structure.  */
struct fat_de_struct
{
    char            name[8];      //!< Filename
    char            ext[3];       //!< Extension, blank filled 
    uint8_t         attr;         //!< File attributes 
    uint8_t         lowerCase;    //!< NT VFAT lower case flags 
    uint8_t         CHundredth;   //!< Hundredth of seconds in CTime 
    uint8_t         CTime[2];     //!< Creation time 
    uint8_t         CDate[2];     //!< Creation date 
    uint8_t         ADate[2];     //!< Last access date 
    uint16_t        cluster_high; //!< High bytes of cluster number 
    uint8_t         MTime[2];     //!< Last update time 
    uint8_t         MDate[2];     //!< Last update date 
    uint16_t        cluster_low;  //!< Starting cluster of file 
    uint32_t        size;         //!< Size of file in bytes 
} __packed__;


typedef struct fat_de_struct fat_de_t;


/** Win95 long name directory entry.  */
struct winentry 
{
    uint8_t         weCnt;          //!< LFN sequence counter
    uint8_t         wePart1[10];    //!< Characters 1-5 of LFN
    uint8_t         weAttributes;   //!< Attributes, must be ATTR_LONG_FILENAME
    uint8_t         weReserved1;    //!< Must be zero, reserved
    uint8_t         weChksum;       //!< Checksum of name in SFN entry
    uint8_t         wePart2[12];    //!< Character 6-11 of LFN
    uint16_t        weReserved2;    //!< Must be zero, reserved
    uint8_t         wePart3[4];     //!< Character 12-13 of LFN
} __packed__;


/** FAT directory entry iterator structure.  */
struct fat_de_iter_struct
{
    fat_t *fs;
    uint16_t sectors;           //!< Number of sectors per dir cluster
    uint32_t cluster;           //!< Current dir cluster
    fat_dir_t dir;              //!< Current dir sector and offset
};


typedef struct fat_de_iter_struct fat_de_iter_t;


/** Return number of sectors for a directory.  */
static int
fat_dir_sector_count (fat_t *fat, uint32_t cluster)
{
    if (fat->type == FAT_FAT16 && cluster == fat->root_dir_cluster)
        return fat->root_dir_sectors; 
    else
        return fat->sectors_per_cluster;
}


static fat_de_t *
fat_de_first (fat_t *fat, uint32_t cluster, fat_de_iter_t *de_iter)
{
    de_iter->fs = fat;
    de_iter->cluster = cluster;
    de_iter->dir.sector = fat_cluster_to_sector (fat, cluster);
    de_iter->dir.offset = 0;
    de_iter->sectors = fat_dir_sector_count (fat, cluster);    

    return (fat_de_t *) fat_io_cache_read (fat, de_iter->dir.sector);
}


static bool
fat_de_last_p (const fat_de_t *de)
{
    /* The end of a directory is marked by an empty slot.  */
    return de == NULL || de->name[0] == SLOT_EMPTY;
}


static fat_de_t *
fat_de_next (fat_de_iter_t *de_iter)
{
    fat_t *fat;
    uint8_t *buffer;

    fat = de_iter->fs;

    de_iter->dir.offset += sizeof (fat_de_t);

    /* There is a chance that will read wrong sector if need
       to read next sector but we fix this up later.  */
    buffer = fat_io_cache_read (fat, de_iter->dir.sector);
    /* Something has gone wrong so give up.  */
    if (!buffer)
        return 0;

    if (de_iter->dir.offset >= fat->bytes_per_sector)
    {
        de_iter->dir.offset = 0;
        de_iter->dir.sector++;

        if (de_iter->dir.sector % de_iter->sectors == 0)
        {
            uint32_t cluster_next;

            /* If reached end of current cluster, find next cluster in
               chain.  */
            cluster_next = fat_cluster_next (fat, de_iter->cluster);

            if (fat_cluster_last_p (cluster_next))
            {
                uint32_t sector;

                /* Have reached end of chain.  Normally we will have
                   found the empty slot terminator.  If we get here we
                   want another cluster added to the directory.  */
                cluster_next = fat_cluster_chain_extend (fat, de_iter->cluster, 1);

                de_iter->dir.sector = fat_cluster_to_sector (fat, cluster_next);

                /* The linux kernel (see linux/fs/fat/dir.c) searches
                   all the directory entries in the cluster chain and
                   does not stop when it finds a zero entry.  So we
                   zero all the sectors for this cluster creating
                   empty slots; in reverse order so the last one we
                   want is in the cache.  */
                for (sector = de_iter->sectors; sector > 0; sector--)
                {
                    buffer = fat_io_cache_read (fat, de_iter->dir.sector 
                                                + sector - 1);
                    /* Something has gone wrong so give up.  */
                    if (!buffer)
                        return 0;
                    memset (buffer, 0, FAT_SECTOR_SIZE);
                    fat_io_cache_write (fat, de_iter->dir.sector 
                                        + sector - 1);                    
                }
            }

            de_iter->cluster = cluster_next;
            de_iter->dir.sector = fat_cluster_to_sector (fat, de_iter->cluster);
        }

        buffer = fat_io_cache_read (fat, de_iter->dir.sector);
    }

    return (fat_de_t *) (buffer + de_iter->dir.offset);
}


static inline bool
fat_de_free_p (const fat_de_t *de)
{
    return de->name[0] == SLOT_DELETED;
}


static inline bool
fat_de_attr_long_filename_p (const fat_de_t *de)
{
    return (de->attr & ATTR_LONG_FILENAME) == ATTR_LONG_FILENAME;
}


static inline bool
fat_de_attr_volume_p (const fat_de_t *de)
{
    return (de->attr & ATTR_VOLUME) == ATTR_VOLUME;
}


static inline bool
fat_de_attr_dir_p (const fat_de_t *de)
{
    return (de->attr & ATTR_DIRECTORY) == ATTR_DIRECTORY;
}


static uint8_t 
fat_de_filename_entries (const char *filename)
{
    return (strlen (filename) + WIN_CHARS - 1) / WIN_CHARS;
}


/**
 * Compare two filenames ignoring case.
 * @param name1 Name 1
 * @param name2 Name 2
 * @return non-zero if match.
 */
static bool
fat_de_filename_match_p (const char *name1, const char *name2)
{
    return strcasecmp (name1, name2) == 0;
}


/**
 * Make filename from DOS short name
 * 
 * @param str  Filename buffer (at least 8 + 3 + 1 + 1 = 13 chars long)
 * @param name DOS name
 * @param ext  DOS extension
 */
static void 
fat_de_filename_make (char *str, const char *name, const char *ext)
{
    uint8_t i;

    for (i = 0; i < 8 && *name && *name != ' '; i++)
        *str++ = *name++;

    if (*ext && *ext != ' ')
    {
        *str++ = '.';
        for (i = 0; i < 3 && *ext && *ext != ' '; i++)
            *str++ = *ext++;
    }
    *str = 0;
}

/** Create short filename entry.  */
static void
fat_de_sfn_create (fat_de_t *de, const char *filename)
{
    int i;
    char *str;

    str = de->name;
    for (i = 0; i < 8; i++)
    {
        if (*filename == '.' || !*filename)
            break;
        *str++ = toupper (*filename++);
    }
    for (; i < 8; i++)
        *str++ = ' ';

    while (*filename && *filename != '.')
        filename++;
    if (*filename == '.')
        filename++;

    str = de->ext;
    for (i = 0; i < 3; i++)
    {
        if (!*filename)
            break;
        *str++ = toupper (*filename++);
    }
    for (; i < 3; i++)
        *str++ = ' ';        

    /* Set dates to 1980 unless can conjure up the date with a RTC.  */
    de->CHundredth = 0x00;
    de->CTime[1] = de->CTime[0] = 0x00;
    de->CDate[1] = 0x00;
    de->CDate[0] = 0x20;
    de->ADate[1] = 0x00;
    de->ADate[0] = 0x20;
    de->MTime[1] = de->MTime[0] = 0x00;
    de->MDate[1] = 0x00;
    de->MDate[0] = 0x20;	
    
    de->lowerCase = 0;

    /* Has been changed since last backup.  */
    de->attr = ATTR_ARCHIVE;
    
    /* These fields get filled in when file written to.  */
    de->cluster_high = 0;
    de->cluster_low = 0;
    de->size = 0;
}


/**
 * Search through disk directory to find the given file or directory
 * 
 * @param dir_cluster Cluster of directory to search
 * @param name File or directory name
 * @param ff File info structure
 * @return non-zero if found
 */
bool
fat_de_find (fat_t *fat, uint32_t dir_cluster, 
             const char *name, fat_ff_t *ff)
{
    fat_de_iter_t de_iter;
    fat_de_t *de;
    bool match = 0;
    bool longmatch = 0;
    char name1[WIN_CHARS];
    uint8_t n;
    uint8_t nameoffset;

    TRACE_INFO (FAT, "FAT:Search %s\n", name);

    memset (ff->name, 0, sizeof (ff->name));
    memset (ff->short_name, 0, sizeof (ff->short_name));

    /* Iterate over direntry in current directory.  */
    for (de = fat_de_first (fat, dir_cluster, &de_iter);
         !fat_de_last_p (de); de = fat_de_next (&de_iter))
    {
        if (fat_de_free_p (de))
            continue;

        /* With long filenames there are a bunch of 32-byte entries
           that precede the normal short entry.  Each of these
           entries stores part of the long filename.  */

        if (fat_de_attr_long_filename_p (de))
        {
            struct winentry *we = (struct winentry *)de;

            if (we->weCnt & WIN_LAST)
                memset (ff->name, 0, sizeof (ff->name));
            
            /* Piece together a fragment of the long name and place it
               at the correct spot.  */
            nameoffset = ((we->weCnt & WIN_CNT) - 1) * WIN_CHARS;
            
            for (n = 0; n < 5; n++)
                ff->name[nameoffset + n] = we->wePart1[n * 2];
            for (n = 0; n < 6; n++)
                ff->name[nameoffset + 5 + n] = we->wePart2[n * 2];
            for (n = 0; n < 2; n++)
                ff->name[nameoffset + 11 + n] = we->wePart3[n * 2];
            
            /* Check for end of long name.  */
            if ((we->weCnt & WIN_CNT) == 1)
                longmatch = fat_de_filename_match_p (name, ff->name);
        }
        else
        {
            /* Found short name entry, convert to filename.  */
            fat_de_filename_make (name1, de->name, de->ext);
            match = fat_de_filename_match_p (name, name1);

            if (strcmp (name1, ".") != 0)
            {
                if ((match || longmatch) && ! fat_de_attr_volume_p (de))
                {
                    /* File found.  */
                    ff->dir = de_iter.dir;

                    if (!longmatch)
                        strcpy (ff->name, name1);

                    ff->cluster = le16_to_cpu (de->cluster_high << 16)
                        | le16_to_cpu (de->cluster_low);

                    ff->size = le32_to_cpu (de->size);

                    ff->isdir = fat_de_attr_dir_p (de);

                    return 1;
                }
            }
        }
    }
    return 0;
}


static void
fat_de_dump (fat_t *fat, fat_de_t *de)
{
    char filename[14];
    uint32_t cluster;

    fat_de_filename_make (filename, de->name, de->ext);

    cluster = (de->cluster_high << 16) | de->cluster_low;

    TRACE_ERROR (FAT, "%s attr %02x size %d clusters ",
                 filename, de->attr, (unsigned int)de->size);

    fat_cluster_chain_dump (fat, cluster);
}


void
fat_de_dir_dump (fat_t *fat, uint32_t dir_cluster)
{
    fat_de_iter_t de_iter;
    fat_de_t *de;

    TRACE_ERROR (FAT, "Dir cluster %" PRIu32 "\n", dir_cluster);
    for (de = fat_de_first (fat, dir_cluster, &de_iter);
         !fat_de_last_p (de); de = fat_de_next (&de_iter))
    {
        if (fat_de_free_p (de))
            TRACE_ERROR (FAT, "Empty slot\n");            
        else if (fat_de_attr_dir_p (de))
            fat_de_dir_dump (fat,
                             (de->cluster_high << 16) | de->cluster_low);
        else
            fat_de_dump (fat, de);
    }
}

             
void
fat_de_size_set (fat_t *fat, fat_dir_t *dir, uint32_t size)
{
    uint8_t *buffer;
    fat_de_t *de;

    buffer = fat_io_cache_read (fat, dir->sector);
    de = (fat_de_t *) (buffer + dir->offset);
    de->size = cpu_to_le32 (size);
    fat_io_cache_write (fat, dir->sector);

    /* Note, the cache needs flushing for this to take effect.  */
}


bool
fat_de_add (fat_t *fat, fat_dir_t *dir, const char *filename,
            uint32_t cluster_dir)
{
    int entries;
    fat_de_iter_t de_iter;
    fat_de_t *de;

    /* With 512 bytes per sector, 1 sector per cluster, and 32 bytes.  */
    /* per dir entry then there 16 slots per cluster.  */

    TRACE_INFO (FAT, "FAT:Add %s\n", filename);

    /* Iterate over direntry in current directory looking for a free slot.  */
    for (de = fat_de_first (fat, cluster_dir, &de_iter);
         !fat_de_last_p (de); de = fat_de_next (&de_iter))
    {
        if (fat_de_free_p (de))
            break;
    }

    /* TODO, what if we find a slot but it is not big enough?.  */

    entries = fat_de_filename_entries (filename);
    if (entries > 1)
    {
        /* TODO.  We need to allocate space in the directory for the
         long filename.  */
        TRACE_ERROR (FAT, "FAT:Long filename\n");        
    }
    
    /* Record where dir entry is.  */
    *dir = de_iter.dir;

    if (fat_de_last_p (de))
    {
        fat_de_t *de_next;

        /* This will create a new cluster if at end of current one
           with an empty slot.  */
        de_next = fat_de_next (&de_iter);
        if (!de_next)
        {
            /* Must have run out of memory.  */
            TRACE_ERROR (FAT, "FAT:Dir extend fail\n");
            return 0;
        }
        fat_io_cache_read (fat, dir->sector);
    }

    /* Create short filename entry.  */
    fat_de_sfn_create (de, filename);

    fat_io_cache_write (fat, dir->sector);
    fat_io_cache_flush (fat);
    return 1;
}


void
fat_de_cluster_set (fat_t *fat, fat_dir_t *dir, uint32_t cluster)
{
    uint8_t *buffer;
    fat_de_t *de;

    buffer = fat_io_cache_read (fat, dir->sector);
    de = (fat_de_t *) (buffer + dir->offset);
    de->cluster_high = cpu_to_le16 (cluster >> 16);
    de->cluster_low = cpu_to_le16 (cluster);
    fat_io_cache_write (fat, dir->sector);

    /* Note, the cache needs flushing for this to take effect.  */
}


void
fat_de_slot_delete (fat_t *fat, fat_dir_t *dir, uint32_t cluster)
{
    fat_de_iter_t de_iter;
    fat_de_t *de;

    /* Search for start of desired dir entry.  */
    for (de = fat_de_first (fat, cluster, &de_iter);
         !fat_de_last_p (de); de = fat_de_next (&de_iter))
    {
        if (de_iter.dir.offset == dir->offset 
            && de_iter.dir.sector == dir->sector)
        {
            for (; fat_de_attr_long_filename_p (de);
                 de = fat_de_next (&de_iter))
            {
                de->name[0] = SLOT_DELETED;                
                fat_io_cache_write (fat, de_iter.dir.sector); 
            }
            
            de->name[0] = SLOT_DELETED;     
            fat_io_cache_write (fat, de_iter.dir.sector);           
            fat_io_cache_flush (fat);
        }
    }
}

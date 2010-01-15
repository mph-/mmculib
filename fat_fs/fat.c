#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <ctype.h>
#include "trace.h"
#include "fat.h"

/* For a simplified description of FAT32 see 
   http://www.pjrc.com/tech/8051/ide/fat32.html
*/


#ifndef TRACE_FAT_ERROR
#define TRACE_FAT_ERROR(...)
#endif


#ifndef TRACE_FAT_INFO
#define TRACE_FAT_INFO(...)
#endif


#ifndef TRACE_FAT_DEBUG
#define TRACE_FAT_DEBUG(...)
#endif

/* The maximum length of a filename that we will consider.  A bigger
   value just gobbles more temporary storage when parsing
   pathnames.  */
#ifndef FAT_MAXLEN_USE
#define FAT_MAXLEN_USE 32
#endif


/* The maximum length of a filename.  */
#define FAT_MAXLEN 256

#define FAT_SECTOR_SIZE 512

/* Size of a winentry.  */
#define WIN_CHARS 13


/* Maximum filename length in Win95.  */
#define WIN_MAXLEN 255


/**
 * \name Some useful cluster numbers.
 * 
 */
//@{
#define MSDOSFSROOT     0               //!< Cluster 0 means the root dir 
#define CLUST_FREE      0               //!< Cluster 0 also means a free cluster 
#define MSDOSFSFREE     CLUST_FREE      //!< Cluster 0 also means a free cluster
#define CLUST_FIRST     2               //!< First legal cluster number 
#define CLUST_RSRVD     0xfffffff6      //!< Reserved cluster range 
#define CLUST_BAD       0xfffffff7      //!< A cluster with a defect 
#define CLUST_EOFS      0xfffffff8      //!< Start of eof cluster range 
#define CLUST_EOFE      0xffffffff      //!< End of eof cluster range 
//@}


#define FAT12_MASK      0x00000fff      //!< Mask for 12 bit cluster numbers 
#define FAT16_MASK      0x0000ffff      //!< Mask for 16 bit cluster numbers 
#define FAT32_MASK      0x0fffffff      //!< Mask for FAT32 cluster numbers 


/**
 * \name Partition Types
 *  
 */
//@{ 
#define PART_TYPE_UNKNOWN   0x00
#define PART_TYPE_FAT12     0x01
#define PART_TYPE_XENIX     0x02
#define PART_TYPE_DOSFAT16  0x04
#define PART_TYPE_EXTDOS    0x05
#define PART_TYPE_FAT16     0x06
#define PART_TYPE_NTFS      0x07
#define PART_TYPE_FAT32     0x0B
#define PART_TYPE_FAT32LBA  0x0C
#define PART_TYPE_FAT16LBA  0x0E
#define PART_TYPE_EXTDOSLBA 0x0F
#define PART_TYPE_ONTRACK   0x33
#define PART_TYPE_NOVELL    0x40
#define PART_TYPE_PCIX      0x4B
#define PART_TYPE_PHOENIXSAVE   0xA0
#define PART_TYPE_CPM       0xDB
#define PART_TYPE_DBFS      0xE0
#define PART_TYPE_BBT       0xFF
//@}


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
 * This is the format of the contents of the deDate field in the direntry
 * structure.
 */
#define DD_DAY_MASK             0x1F    //!< day of month 
#define DD_DAY_SHIFT            0       //!< -
#define DD_MONTH_MASK           0x1E0   //!< month 
#define DD_MONTH_SHIFT          5       //!< -
#define DD_YEAR_MASK            0xFE00  //!< year - 1980 
#define DD_YEAR_SHIFT           9       //!< -


/** \struct partrecord 
 * Partition Record Structure
 * 
 */ 
struct partrecord /* length 16 uint8_ts */
{           
    uint8_t     prIsActive;                 //!< 0x80 indicates active partition 
    uint8_t     prStartHead;                //!< Atarting head for partition 
    uint16_t    prStartCylSect;             //!< Atarting cylinder and sector 
    uint8_t     prPartType;                 //!< Partition type (see above) 
    uint8_t     prEndHead;                  //!< Ending head for this partition 
    uint16_t    prEndCylSect;               //!< Ending cylinder and sector 
    uint32_t    prStartLBA;                 //!< First LBA sector for this partition 
    uint32_t    prSize;                     //!< Size of this partition (uint8_ts or sectors ?)
} __packed__;

        
/* The first sector (512 bytes) of the drive is the MBR.  This
   contains 446 bytes of boot code followed by a 64 byte partition
   table comprised of 4 16 byte primary partition entries.  The MBR is
   terminated by 0x55aa.  The MBR can only represent 4 partitions.
   The first sector of a FAT file system is called the volume ID.
   This describes the layout of the FAT file system.  After the volume
   ID are some reserved sectors followed by two copies of the FAT.
   The remainder of the file system is data arranged in clusters with
   perhaps some unused space at the end.  The clusters hold the files
   and directories.  Clusters are numbered from 2.  Normally clusters
   are 8 sectors (4K) but 8K, 16K, and 32K are used.
*/


/** \struct partsector
 *  Partition Sector
 * 
 */        
struct partsector
{
    uint8_t         psBoot[446]; 
    struct          partrecord psPart[4];   //!< Four partition records (64 bytes)
    uint8_t         psBootSectSig0;         //!< First signature   
    uint8_t         psBootSectSig1;         //!< Second signature
#define BOOTSIG0    0x55                    //!< Signature constant 0
#define BOOTSIG1    0xaa                    //!< Signature constant 1
} __packed__;


/** \struct bpb710 
 * BPB for DOS 7.10 (FAT32).  This one has a few extensions to bpb50.
 * 
 */
struct bpb710 
{
    uint16_t        bpbBytesPerSec; //!< Bytes per sector 
    uint8_t         bpbSecPerClust; //!< Sectors per cluster 
    uint16_t        bpbResSectors;  //!< Number of reserved sectors 
    uint8_t         bpbFATs;        //!< Number of FATs 
    uint16_t        bpbRootDirEnts; //!< Number of root directory entries 
    uint16_t        bpbSectors;     //!< Total number of sectors 
    uint8_t         bpbMedia;       //!< Media descriptor 
    uint16_t        bpbFATsecs;     //!< Number of sectors per FAT
    uint16_t        bpbSecPerTrack; //!< Sectors per track
    uint16_t        bpbHeads;       //!< Number of heads 
    uint32_t        bpbHiddenSecs;  //!< Number of hidden sectors
// 3.3 compat ends here 
    uint32_t        bpbHugeSectors; //!< Number of sectors if bpbSectors == 0
// 5.0 compat ends here 
    uint32_t        bpbBigFATsecs;  //!< Like bpbFATsecs for FAT32
    uint16_t        bpbExtFlags;    //!< Extended flags:
#define FATNUM          0xf             //!< Mask for numbering active FAT
#define FATMIRROR       0x80            //!< FAT is mirrored (like it always was)
    uint16_t        bpbFSVers;      //!< Filesystem version
#define FSVERS          0               //!< Currently only 0 is understood
    uint32_t        bpbRootClust;   //!< Start cluster for root directory
    uint16_t        bpbFSInfo;      //!< Filesystem info structure sector
    uint16_t        bpbBackup;      //!< Backup boot sector 
    uint8_t         bppFiller[12];  //!< Reserved
} __packed__;


/** \struct extboot
 * Fat12 and Fat16 boot block structure
 * 
 */ 
struct extboot
{
    char          exDriveNumber;        //!< Drive number (0x80)
    char          exReserved1;          //!< Reserved
    char          exBootSignature;      //!< Ext. boot signature (0x29)
#define EXBOOTSIG       0x29                //!< Extended boot signature                
    char          exVolumeID[4];        //!< Volume ID number
    char          exVolumeLabel[11];    //!< Volume label
    char          exFileSysType[8];     //!< File system type (FAT12 or FAT16)
} __packed__;


/** \struct bootsector710 
 * Boot Sector.
 * This is the first sector on a DOS floppy disk or the first sector of a partition 
 * on a hard disk.  But, it is not the first sector of a partitioned hard disk.
 * 
 */
struct bootsector710 
{
    uint8_t         bsJump[3];              //!< Jump inst E9xxxx or EBxx90 
    char            bsOEMName[8];           //!< OEM name and version 
    // 56
    struct bpb710   bsBPB;                  //!< BPB block
    // 26
    struct extboot  bsExt;                  //!< Bootsector Extension 

    char            bsBootCode[418];        //!< Pad so structure is 512
    uint8_t         bsBootSectSig2;         //!< 2 & 3 are only defined for FAT32? 
    uint8_t         bsBootSectSig3;         //!< 2 & 3 are only defined for FAT32?
    uint8_t         bsBootSectSig0;         //!< 2 & 3 are only defined for FAT32?
    uint8_t         bsBootSectSig1;         //!< 2 & 3 are only defined for FAT32?

#define BOOTSIG0        0x55                    //!< Signature constant 0
#define BOOTSIG1        0xaa                    //!< Signature constant 1
#define BOOTSIG2        0                       //!< Signature constant 2
#define BOOTSIG3        0                       //!< Signature constant 3
} __packed__;


/** \struct fsinfo
 *  FAT32 FSInfo block.
 * 
 */
struct fsinfo
{
    uint8_t fsisig1[4];         //!< Lead signature
    uint8_t fsifill1[480];      //!< Reseved
    uint8_t fsisig2[4];         //!< Structure signature
    uint8_t fsinfree[4];        //!< Last known free cluster count on the volume
    uint8_t fsinxtfree[4];      //!< Indicates the cluster number at which the driver should start looking for free clusters
    uint8_t fsifill2[12];       //!< Reserved 
    uint8_t fsisig3[4];         //!< Trail signature
    uint8_t fsifill3[508];      //!< Reserved
    uint8_t fsisig4[4];         //!< Sector signature 0xAA55
} __packed__;


/** 
 * \struct fatdirentry
 * DOS Directory entry.
 * 
 */
struct fatdirentry
{
    char            name[8];      //!< Filename, blank filled 
#define SLOT_EMPTY      0x00            //!< Slot has never been used 
#define SLOT_E5         0x05            //!< The real value is 0xe5 
#define SLOT_DELETED    0xe5            //!< File in this slot deleted 
    char            extension[3]; //!< Extension, blank filled 
    char            attributes;   //!< File attributes 
#define ATTR_NORMAL     0x00            //!< Normal file 
#define ATTR_READONLY   0x01            //!< File is readonly 
#define ATTR_HIDDEN     0x02            //!< File is hidden 
#define ATTR_SYSTEM     0x04            //!< File is a system file 
#define ATTR_VOLUME     0x08            //!< Entry is a volume label 
#define ATTR_LONG_FILENAME  0x0f        //!< This is a long filename entry              
#define ATTR_DIRECTORY  0x10            //!< Entry is a directory name 
#define ATTR_ARCHIVE    0x20            //!< File is new or modified 
    uint8_t         lowerCase;    //!< NT VFAT lower case flags 
#define LCASE_BASE      0x08            //!< Filename base in lower case 
#define LCASE_EXT       0x10            //!< Filename extension in lower case 
    uint8_t         CHundredth;   //!< Hundredth of seconds in CTime 
    uint8_t         CTime[2];     //!< Creation time 
    uint8_t         CDate[2];     //!< Creation date 
    uint8_t         ADate[2];     //!< Last access date 
    uint16_t        cluster_high;  //!< High bytes of cluster number 
    uint8_t         MTime[2];     //!< Last update time 
    uint8_t         MDate[2];     //!< Last update date 
    uint16_t        cluster_low; //!< Starting cluster of file 
    uint32_t        file_size;     //!< Size of file in bytes 
} __packed__;


/**
 * \typedef fatdirentry_t 
 * Type definition for fatdirentry
 * 
 */
typedef struct fatdirentry  fatdirentry_t;



typedef struct
{
    uint32_t        parent_dir_cluster;
    uint32_t        cluster;
    uint32_t        dir_sector;
    uint32_t        dir_offset;
    bool            islong;      //!< Set to indicate a long name
    bool            isdir;       //!< Set to indicate a directory
    char            short_name[12]; //!< DOS'ified version of the (short) name we're looking for   
    char            name[FAT_MAXLEN]; //!< Name entry found (long or short) 
    fatdirentry_t   de;          //!< directory entry with file data (short entry)
} fat_ff_t;              


/**
 * \typedef fat_t
 * Filehandle structure.
 * 
 */
struct fat_struct 
{
    fat_fs_t        *fs;
    int             mode;           //!< file mode
    uint32_t        file_offset;        //!< file offset
    uint32_t        file_size;      //!< max offset used and/or filesize when reading
    uint32_t        start_cluster;   //!< starting cluster of file
    uint32_t        cluster;        //!< current cluster 
    uint32_t        dir_sector;
    uint32_t        dir_offset;
};


/** \struct winentry
 *  Win95 long name directory entry.
 */
struct winentry 
{
    uint8_t         weCnt;          //!< LFN sequence counter
#define WIN_LAST        0x40        //!< Last sequence indicator
#define WIN_CNT         0x3f        //!< Sequence number mask
    uint8_t         wePart1[10];    //!< Characters 1-5 of LFN
    uint8_t         weAttributes;   //!< Attributes, must be ATTR_LONG_FILENAME
#define ATTR_WIN95      0x0f        //!< Attribute of LFN for Win95
    uint8_t         weReserved1;    //!< Must be zero, reserved
    uint8_t         weChksum;       //!< Checksum of name in SFN entry
    uint8_t         wePart2[12];    //!< Character 6-11 of LFN
    uint16_t        weReserved2;    //!< Must be zero, reserved
    uint8_t         wePart3[4];     //!< Character 12-13 of LFN
} __packed__;


/*
 * 4 byte uint8_t to uint32_t conversion union
 * 
 */
typedef union
{
    uint32_t ulong;     //!< uint32_t result
    uint8_t uchar[4];   //!< 4 byte uint8_t to convert
} LBCONV;


struct fat_fs_struct
{
    void *msd;
    uint8_t     isFAT32;                //!< FAT32 / FAT16 indicator
    uint16_t    sectors_per_cluster;    //!< Number of sectors in each disk cluster
    uint32_t    first_data_sector;      //!< LBA index of first sector of the dataarea on the disk
    uint32_t    first_fat_sector;       //!< LBA index of first FAT sector on the disk
    uint32_t    first_dir_sector;       //!< LBA index of first Root Directory sector on the disk
    uint32_t    root_dir_cluster;       //!< First Cluster of Directory (FAT32)
    uint16_t    root_dir_sectors;       //!< Number of Sectors in Root Dir (FAT16)
    uint32_t    num_clusters;           //!< Number of data clusters on partition
    uint32_t    num_fat_sectors;        //!< Number of sectors per FAT
    uint32_t    current_dir_cluster;    //!< Cluster of working directory
    uint16_t    bytes_per_sector;       //!< Number of bytes per sector
    uint16_t    bytes_per_cluster;      //!< Number of bytes per cluster
    uint32_t    sector;                  //!< Cached sector
    uint8_t     sector_buffer[FAT_SECTOR_SIZE];
    bool        dirty;
};

#ifndef FAT_NUM
#define FAT_NUM 1
#endif

static fat_fs_t fat_info[FAT_NUM];
static uint8_t fat_num;


typedef uint32_t fat_sector_t;

static msd_size_t
fat_raw_read (fat_fs_t *fat_fs, msd_addr_t addr, void *buffer, msd_size_t size)
{
    return msd_read (fat_fs->msd, addr, buffer, size);
}


static msd_size_t
fat_raw_write (fat_fs_t *fat_fs, msd_addr_t addr, const void *buffer, msd_size_t size)
{
    return msd_write (fat_fs->msd, addr, buffer, size);
}


static msd_size_t
fat_sector_read (fat_fs_t *fat_fs, fat_sector_t sector, void *buffer)
{
    return fat_raw_read (fat_fs, sector * fat_fs->bytes_per_sector, buffer, 
                         fat_fs->bytes_per_sector);
}


static msd_size_t
fat_sector_write (fat_fs_t *fat_fs, fat_sector_t sector, const void *buffer)
{
    return fat_raw_write (fat_fs, sector * fat_fs->bytes_per_sector, buffer, 
                          fat_fs->bytes_per_sector);
}


static void
fat_sector_cache_flush (fat_fs_t *fat_fs)
{
    if (fat_fs->dirty)
    {
        fat_sector_write (fat_fs, fat_fs->sector, fat_fs->sector_buffer);
        fat_fs->dirty = 0;
    }
}


static msd_size_t
fat_sector_cache_read (fat_fs_t *fat_fs, fat_sector_t sector)
{
    if (sector == fat_fs->sector)
        return fat_fs->bytes_per_sector;

    fat_sector_cache_flush (fat_fs);

    fat_fs->sector = sector;
    return fat_sector_read (fat_fs, sector, fat_fs->sector_buffer);
}


static msd_size_t
fat_sector_cache_write (fat_fs_t *fat_fs, fat_sector_t sector)
{
    fat_fs->sector = sector;
    fat_fs->dirty = 1;
    /* Don't write through to device; need to call fat_sector_cache_flush
       when finished.  */
    return fat_fs->bytes_per_sector;
}


static msd_size_t
fat_partial_sector_read (fat_fs_t *fat_fs, fat_sector_t sector,
                         msd_size_t offset, void *buffer, msd_size_t size)
{
    return fat_raw_read (fat_fs, sector * fat_fs->bytes_per_sector + offset, 
                         buffer, size);
}


static msd_size_t
fat_partial_sector_write (fat_fs_t *fat_fs, fat_sector_t sector,
                          msd_size_t offset, const void *buffer, msd_size_t size)
{
    return fat_raw_write (fat_fs, sector * fat_fs->bytes_per_sector + offset, 
                          buffer, size);
}


/**
 * Cluster to sector
 * 
 * Converting a cluster number to a LBA sector number.
 * \param clust Cluster number
 * \return Sector number
 * 
 */
static uint32_t
fat_sector_calc (fat_fs_t *fat_fs, uint32_t cluster)
{
    // If this is a cluster request for the rootdir, point to it
    if (cluster == 0)
        return fat_fs->first_dir_sector;

    // Clusters are numbered starting from 2
    return ((uint32_t) (cluster - 2) * fat_fs->sectors_per_cluster) 
        + fat_fs->first_data_sector;    
}


/**
 * Next cluster
 * 
 * Find next cluster in the FAT chain.
 * 
 * \param   cluster     Actual cluster
 * \return  Next cluster in chain
 * 
 */
static uint32_t 
fat_entry_get (fat_fs_t *fat_fs, uint32_t cluster)
{
    uint32_t sector, offset, cluster_new, mask;
    LBCONV conv;
    
    // Calculate the sector number and sector offset in the FAT for
    // this cluster number
    
    if (fat_fs->isFAT32)
    {
        // There are 4 bytes per FAT entry in FAT32  
        offset = cluster << 2;
        mask = FAT32_MASK;
    }
    else
    {
        // There are 2 bytes per FAT entry in FAT16
        offset = cluster << 1;
        mask = FAT16_MASK;
    }

    // Read sector of FAT#1 for desired cluster entry
    sector = fat_fs->first_fat_sector + (offset / fat_fs->bytes_per_sector);
    fat_sector_cache_read (fat_fs, sector);
    
    // Get the data for desired FAT entry
    offset = offset % fat_fs->bytes_per_sector;
    conv.uchar[0] = fat_fs->sector_buffer[offset]; 
    conv.uchar[1] = fat_fs->sector_buffer[offset + 1];  
    if (fat_fs->isFAT32)
    {
        conv.uchar[2] = fat_fs->sector_buffer[offset + 2];  
        conv.uchar[3] = fat_fs->sector_buffer[offset + 3];  
    }

    cluster_new = conv.ulong & mask;

    return cluster_new & mask;
}


/* Return true if cluster is the last in the chain.  */
static bool
fat_cluster_last_p (uint32_t cluster)
{
    return cluster == CLUST_EOFE;
}


/* Return the next cluster in the chain.  */
uint32_t 
fat_cluster_next (fat_fs_t *fat_fs, uint32_t cluster)
{
    if (fat_cluster_last_p (cluster))
        return cluster;

    if (cluster < 2)
        cluster = 2;
    
    return fat_entry_get (fat_fs, cluster);
}


static void
fat_entry_set (fat_fs_t *fat_fs, uint32_t cluster, uint32_t cluster_new)
{
    uint32_t sector, offset, mask;
    LBCONV conv;
    
    // Calculate the sector number and sector offset in the FAT for
    // this cluster number
    
    if (fat_fs->isFAT32)
    {
        // There are 4 bytes per FAT entry in FAT32  
        offset = cluster << 2;
        mask = FAT32_MASK;
    }
    else
    {
        // There are 2 bytes per FAT entry in FAT16
        offset = cluster << 1;
        mask = FAT16_MASK;
    }

    // Read sector of FAT#1 for desired cluster entry
    sector = fat_fs->first_fat_sector + (offset / fat_fs->bytes_per_sector);
    fat_sector_cache_read (fat_fs, sector);

    conv.ulong = cluster_new;
    
    // Set the data for desired FAT entry
    offset = offset % fat_fs->bytes_per_sector;
    fat_fs->sector_buffer[offset] = conv.uchar[0];
    fat_fs->sector_buffer[offset + 1] = conv.uchar[1];
        
    if (fat_fs->isFAT32)
    {
        fat_fs->sector_buffer[offset + 2] = conv.uchar[2];
        fat_fs->sector_buffer[offset + 3] = conv.uchar[3];
    }
    fat_sector_cache_write (fat_fs, sector);
}


/**
 * TBD
 * 
 * \todo Add function description here
 * 
 */
static bool 
szWildMatch8 (const char *pat, const char * str) 
{
    const char *s, *p;
    uint8_t star = 0;

loopStart:
    for (s = str, p = pat; *s; ++s, ++p) 
    {
        switch (*p) 
        {
        case '?':
            if (*s == '.') goto starCheck;
            break;
        case '*':
            star = 1;
            str = s, pat = p;
            //do { ++pat; } while (*pat == '*');    // can be skipped
            if (!* (++pat)) return 1;
            goto loopStart;
        default:
            if (toupper (*s) != toupper (*p))
                goto starCheck;
            break;
        } // endswitch 
    } // endfor 
    if (*p == '*') ++p;
    return (!*p);

starCheck:
    if (!star) return 0;
    str++;
    goto loopStart;
}


/**
 * Make valid filename from DOS short name
 * 
 * \param str Filename buffer
 * \param dos DOS filename
 * \param ext Extention
 * 
 */
void dos2str (char *str, const char *dos, const char *ext)
{
    uint8_t i;

    for (i = 0; i < 8 && *dos && *dos != ' '; i++)
        *str++ = *dos++;

    if (*ext && *ext != ' ')
    {
        *str++ = '.';
        for (i=0;i<3 && *ext && *ext != ' ';i++)
            *str++ = *ext++;
    }
    *str = 0;
}


static int
fat_dir_sectors (fat_fs_t *fat_fs, uint32_t cluster)
{
    if (fat_fs->isFAT32 == 0 && cluster == fat_fs->root_dir_cluster)
    {
        // If not FAT32 and this is the root dir set max sector count
        // for root dir
        return fat_fs->root_dir_sectors; 
    }
    return fat_fs->sectors_per_cluster;
}


typedef struct fat_de_iter_struct
{
    fat_fs_t *fs;
    uint32_t cluster;
    uint32_t sector;
    uint16_t sectors;
    uint16_t item;
    uint16_t offset;
    fatdirentry_t *de;    
} fat_de_iter_t;


static fatdirentry_t *
fat_de_first (fat_fs_t *fat_fs, uint32_t cluster, fat_de_iter_t *de_iter)
{
    de_iter->cluster = cluster;
    de_iter->sector = fat_sector_calc (fat_fs, cluster);
    de_iter->sectors = fat_dir_sectors (fat_fs, cluster);    

    fat_sector_cache_read (fat_fs, de_iter->sector);
    de_iter->de = (fatdirentry_t *) fat_fs->sector_buffer;

    de_iter->fs = fat_fs;
    de_iter->item = 0;
    de_iter->offset = 0;

    return de_iter->de;
}


static bool
fat_de_last_p (const fatdirentry_t *de)
{
    // The end of a directory is marked by an empty slot
    return de == NULL || de->name[0] == SLOT_EMPTY;
}


static fatdirentry_t *
fat_de_next (fat_de_iter_t *de_iter)
{
    de_iter->item++;
    de_iter->de++;

    de_iter->offset += sizeof (fatdirentry_t);

    if (de_iter->offset >= de_iter->fs->bytes_per_sector)
    {
        de_iter->offset = 0;
        de_iter->sector++;

        if (de_iter->sector >= de_iter->sectors)
        {
            // If reached end of current cluster, find next cluster in chain.
            de_iter->cluster
                = fat_cluster_next (de_iter->fs, de_iter->cluster);

            // Reached end of chain.
            if (fat_cluster_last_p (de_iter->cluster))
            {
                // Perhaps should add a new cluster?
                TRACE_ERROR (FAT, "FAT:de_next end of chain\n");
                return NULL;
            }

            de_iter->sector = fat_sector_calc (de_iter->fs,
                                                  de_iter->cluster);
        }

        fat_sector_cache_read (de_iter->fs, de_iter->sector);
        de_iter->de = (fatdirentry_t *) de_iter->fs->sector_buffer;
    }

    return de_iter->de;
}


static bool
fat_de_free_p (const fatdirentry_t *de)
{
    return de->name[0] == SLOT_DELETED;
}


static bool
fat_de_attr_long_filename_p (const fatdirentry_t *de)
{
    return (de->attributes & ATTR_LONG_FILENAME) == ATTR_LONG_FILENAME;
}


static bool
fat_long_name_p (const char *name)
{
    char *q;

    // Determine if it's a long or short name
    if ((q = strchr (name, '.')) != 0) // if there is an extension dot 
    {
        if (q - name > 8)             // if the base part is longer than 8 chars
            return 1;

        if (strlen (q + 1) > 3)       // if the extension is longer than 3 chars
            return 1;
    }
    else
    {
        if (strlen (name) > 8)        // if name is longer than 8 chars,
            return 1;
    }
    return 0;
}


/**
 * Scan through disk directory to find the given file or directory
 * 
 * \param name File or directory name
 * \return Error code
 * 
 */
bool
fat_dir_search (fat_fs_t *fat_fs, uint32_t dir_cluster, 
                const char *name, fat_ff_t *ff)
{
    fat_de_iter_t de_iter;
    fatdirentry_t *de;
    bool match = 0;
    bool longmatch = 0;
    char matchspace[13];
    uint8_t n;
    uint8_t nameoffset;

    TRACE_INFO (FAT, "FAT:dir_search for %s\n", name);

    memset (ff->name, 0, sizeof (ff->name));
    memset (ff->short_name, 0, sizeof (ff->short_name));

    ff->islong = fat_long_name_p (name);

    // Iterate over direntry in current directory.
    for (de = fat_de_first (fat_fs, dir_cluster, &de_iter);
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
            
            // Piece together a fragment of the long name
            // and place it at the correct spot
            nameoffset = ((we->weCnt & WIN_CNT) - 1) * WIN_CHARS;
            
            for (n = 0; n < 5; n++)
                ff->name[nameoffset + n] = we->wePart1[n * 2];
            for (n = 0; n < 6; n++)
                ff->name[nameoffset + 5 + n] = we->wePart2[n * 2];
            for (n = 0; n < 2; n++)
                ff->name[nameoffset + 11 + n] = we->wePart3[n * 2];
            
            // Check for end of long name
            if ((we->weCnt & WIN_CNT) == 1)
            {
                // Set match flag
                longmatch = szWildMatch8 (name, ff->name);
            }
        }
        else
        {
            // Found short name entry, determine a match 
            // Note, there is always a short name entry after the long name
            // entries
            dos2str (matchspace, de->name, de->extension);
            match = szWildMatch8 (name, matchspace);

            // Skip the single dot entry
            if (strcmp (matchspace, ".") != 0)
            {
                if ((match || longmatch) 
                    && ((de->attributes & ATTR_VOLUME) == 0))
                {
                    // File found
                    ff->dir_sector = de_iter.sector;
                    ff->dir_offset = de_iter.offset;
                    memcpy (&ff->de, de, sizeof (fatdirentry_t));
                    if (!longmatch)
                    {
                        strcpy (ff->name, matchspace);
                    }
                    ff->cluster = (ff->de.cluster_high << 16)
                        | ff->de.cluster_low;

                    ff->isdir = (ff->de.attributes & ATTR_DIRECTORY)
                        == ATTR_DIRECTORY;
                    return 1;
                }
            }
        }
    }
    return 0;
}


// Search the filesystem for the directory entry for pathname, a file
// or directory
bool
fat_search (fat_fs_t *fat_fs, const char *pathname, fat_ff_t *ff)
{
    char *p, *q;
    char tmp[FAT_MAXLEN_USE];

    if (pathname == NULL || !*pathname)
        return 0;

    p = (char *) pathname;

    // If first char is / then in root directory
    if (*p == '/')
    {
        TRACE_INFO (FAT, "FAT:search: changing to root\n");

        ff->parent_dir_cluster = fat_fs->root_dir_cluster;
        p++;
    }
    else
        ff->parent_dir_cluster = fat_fs->current_dir_cluster;
    
    while (*p)
    {
        // Extract next part of path
        q = tmp;
        while (*p && *p != '/')
            *q++ = *p++;
        *q = 0;

        // Give up if find // within pathname
        if (!*tmp)
            return 0;

        if (!fat_dir_search (fat_fs, ff->parent_dir_cluster, tmp, ff))
        {
            TRACE_ERROR (FAT, "FAT:search %s not found\n", tmp);

            // If this should be a directory but was not found flag
            // parent_dir_cluster as invalid
            if (*p == '/')
                ff->parent_dir_cluster = 0;
            return 0;
        }

        if (*p == '/')
        {
            p++;

            if (!ff->isdir)
            {
                TRACE_ERROR (FAT, "FAT:search %s not a directory\n", tmp);
                return 0;
            }

            if (*p)
                ff->parent_dir_cluster = ff->cluster;
        }
    }

    return 1;
}


/**
 * Change directory
 *
 * \param pathname Directory or path name
 * \return 0 for success -1 for failure
 *  
 */
int
fat_chdir (fat_fs_t *fat_fs, const char *pathname)
{
    fat_ff_t ff;

    // May have fred, fred/, bar/fred, bar/fred/, /foo/fred, /foo/fred/  etc.
    // Need to ignore trailing slash.  If leading slash then search in
    // root directory otherwise search in current directory.

    if (!fat_search (fat_fs, pathname, &ff))
    {
        errno = ENOENT;
        return -1;
    }
    
    if (!ff.isdir)
    {
        errno = ENOTDIR;
        return -1;
    }

    fat_fs->current_dir_cluster = ff.cluster;
    return 0;
}


static fat_t *
fat_find (fat_t *fat, const char *pathname, fat_ff_t *ff)
{
    //   foo/bar    file
    //   foo/bar/   dir
    //   foo/       dir in root dir
    //   foo        file or dir
    //   If remove trailing slash have 2 options
    //   foo/bar    file
    //   foo        file or dir


    if (!fat_search (fat->fs, pathname, ff))
        return NULL;

    TRACE_INFO (FAT, "FAT:found %s\n", pathname);

    fat->start_cluster = ff->cluster;
    fat->cluster = fat->start_cluster;
    fat->file_offset = 0; 
    fat->file_size = ff->de.file_size;
    fat->dir_sector = ff->dir_sector;
    fat->dir_offset = ff->dir_offset;
    return fat;
}


/**
 * Return the size of a file
 * 
 * \param   fat  File handle
 * \return  Size of file
 * 
 */
uint32_t
fat_size (fat_t *fat)
{
    return fat->file_size;
}


void
fat_size_set (fat_t *fat, uint32_t size)
{
    fatdirentry_t *de = (fatdirentry_t *) (fat->fs->sector_buffer
                                           + fat->dir_offset);

    fat_sector_cache_read (fat->fs, fat->dir_sector);
    de->file_size = size;
    fat_sector_cache_write (fat->fs, fat->dir_sector);
}


/**
 * Close a file.
 * 
 * \param fat File handle
 * \return Error code
 * 
 */
int
fat_close (fat_t *fat)
{
    TRACE_INFO (FAT, "FAT:close\n");

    if (fat == NULL)
        return (uint32_t) -1;

    free (fat);
    return 0;
}


/**
 * Read specific number of bytes from file
 * 
 * \param fat File handle
 * \param buffer Buffer to store data
 * \param len Number of bytes to read
 * \return Number of bytes successful read.
 * 
 */
ssize_t
fat_read (fat_t *fat, void *buffer, size_t len)
{
    uint16_t nbytes;
    uint32_t sector;
    uint16_t bytes_left;
    uint16_t offset;
    uint8_t *data;

    TRACE_INFO (FAT, "FAT:read %u\n", (unsigned int)len);
    
    // Limit max read to size of file
    if ((uint32_t)len > (fat->file_size - fat->file_offset))
        len = fat->file_size - fat->file_offset;
    
    data = buffer;
    bytes_left = len;
    while (bytes_left)
    {
        offset = fat->file_offset % fat->fs->bytes_per_sector;
        sector = fat_sector_calc (fat->fs, fat->cluster);

        // Add local sector within a cluster 
        sector += (fat->file_offset % fat->fs->bytes_per_cluster) 
            / fat->fs->bytes_per_sector;

        // Limit to max one sector
        nbytes = bytes_left < fat->fs->bytes_per_sector
            ? bytes_left : fat->fs->bytes_per_sector;

        // Limit to remaining bytes in a sector
        if (nbytes > (fat->fs->bytes_per_sector - offset))
            nbytes = fat->fs->bytes_per_sector - offset;

        // Read the data
        nbytes 
            = fat_partial_sector_read (fat->fs, sector, offset, data, nbytes);

        data += nbytes;

        fat->file_offset += nbytes;
        bytes_left -= nbytes;

        // Cluster boundary
        if (fat->file_offset % fat->fs->bytes_per_cluster == 0)
        {
            // Need to get a new cluster so follow FAT chain
            fat->cluster = fat_cluster_next (fat->fs, fat->cluster);

            // If at end of chain there is no more data
            if (fat_cluster_last_p (fat->cluster))
                break;
        }
    }
    TRACE_INFO (FAT, "FAT:read return %u\n", (unsigned int)len - bytes_left);
    return len - bytes_left;
}


/**
 * Seek to specific position within file.
 * 
 * \param fat File handle
 * \param offset Number of bytes to seek
 * \param whence Position from where to start
 * \return New position within file
 * 
 */
off_t
fat_lseek (fat_t *fat, off_t offset, int whence)
{
    off_t fpos = 0, npos, t;
    uint32_t cluster_new;

    // Setup position to seek from
    switch (whence)
    {
    case SEEK_SET : fpos = 0; break;
    case SEEK_CUR : fpos = fat->file_offset; break;
    case SEEK_END : fpos = fat->file_size; break;
    }
    
    // Adjust and apply limits
    npos = fpos + offset;
    if (npos < 0)
        npos = 0;
    if ((uint32_t)npos > fat->file_size)
        npos = fat->file_size;

    // Now set the new position
    t = npos;
    fat->file_offset = t;

    // Calculate how many clusters from start cluster
    t = npos / fat->fs->bytes_per_cluster;

    // Set start cluster
    fat->cluster = fat->start_cluster;

    // Follow chain
    while (t--)
    {
        cluster_new = fat_cluster_next (fat->fs, fat->cluster);

        if (!fat_cluster_last_p (cluster_new))
            fat->cluster = cluster_new;
        else
            break;
    }

    return npos; 
}


uint16_t 
fat_sector_bytes (fat_fs_t *fat_fs)
{
    return fat_fs->bytes_per_sector;
}


/**
 * Init FAT
 * 
 * Initialize the file system by reading some
 * basic information from the disk
 * 
 */
fat_fs_t *
fat_init (msd_t *msd)
{
    uint32_t tot_sectors;
    uint32_t data_sectors;
    uint32_t first_sector;
    struct partrecord *pr = NULL;
    struct bootsector710 *pb;
    struct bpb710 *bpb;
    struct extboot *bsext;
    fat_fs_t *fat_fs;

    fat_fs = &fat_info[fat_num++];
    if (fat_num > FAT_NUM)
        return NULL;

    fat_fs->msd = msd;
    fat_fs->sector = -1;

    TRACE_INFO (FAT, "FAT:init\n");

    // Read first sector on device.  
    fat_fs->bytes_per_sector = FAT_SECTOR_SIZE;
    fat_sector_cache_read (fat_fs, 0);

    // Check for a jump instruction marking start of MBR
    if (*fat_fs->sector_buffer == 0xE9 || *fat_fs->sector_buffer == 0xEB)
    {
        // Have a boot sector but no partition sector
        first_sector = 0;
        TRACE_ERROR (FAT, "FAT:found MBR, fixme\n");
        return NULL;
    }
    else
    {
        struct partsector *ps;

        // Get the first partition record from the data
        ps = (struct partsector *) fat_fs->sector_buffer;
        pr = (struct partrecord *) ps->psPart;
        // Get partition start sector
        first_sector = pr->prStartLBA;
    }

    fat_fs->isFAT32 = 0;

    /** \todo Add more FAT16 types here.  Note mkdosfs needs -N32
        option to use 32 bits for each FAT otherwise it will default
        to 12 for small systems and this code is not robust enought to
        handle this.  */
    switch (pr->prPartType)
    {
    case PART_TYPE_FAT16:
        TRACE_INFO (FAT, "FAT:FAT16\n");
        break;
        
    case PART_TYPE_FAT32:
    case PART_TYPE_FAT32LBA:
        TRACE_INFO (FAT, "FAT:FAT32\n");
        fat_fs->isFAT32 = 1;
        break;

    default:
        TRACE_INFO (FAT, "FAT:Unknown\n");

        // Most likely a file system hasn't been created.
        return NULL;
    }
    
    // Read Partition Boot Record (Volume ID)
    fat_sector_cache_read (fat_fs, first_sector); 

    // Point to partition boot record
    pb = (struct bootsector710 *) fat_fs->sector_buffer;
    bpb = &pb->bsBPB;
    bsext = &pb->bsExt;
    TRACE_INFO (FAT, "FAT:%s\n", bsext->exVolumeLabel);

    // Number of bytes per sector
    fat_fs->bytes_per_sector = bpb->bpbBytesPerSec;

    // Number of sectors per FAT
    if (bpb->bpbFATsecs != 0)
        fat_fs->num_fat_sectors = bpb->bpbFATsecs;
    else
        fat_fs->num_fat_sectors = bpb->bpbBigFATsecs;

    // Number of sectors in root dir (will be 0 for FAT32)
    fat_fs->root_dir_sectors  
        = ((bpb->bpbRootDirEnts * 32)
            + (bpb->bpbBytesPerSec - 1)) / bpb->bpbBytesPerSec; 

    // First data sector on the volume (partition compensation will be
    // added later)
    fat_fs->first_data_sector = bpb->bpbResSectors + bpb->bpbFATs
        * fat_fs->num_fat_sectors + fat_fs->root_dir_sectors;

    // Calculate total number of sectors on the volume
    if (bpb->bpbSectors != 0)
        tot_sectors = bpb->bpbSectors;
    else
        tot_sectors = bpb->bpbHugeSectors;
        
    // Total number of data sectors
    data_sectors = tot_sectors - fat_fs->first_data_sector;

    // Total number of clusters
    fat_fs->num_clusters = data_sectors / bpb->bpbSecPerClust;

    TRACE_INFO (FAT, "FAT:Data sectors = %ld\n", data_sectors);
    TRACE_INFO (FAT, "FAT:Clusters = %ld\n", fat_fs->num_clusters);

    fat_fs->first_data_sector += first_sector;
    fat_fs->sectors_per_cluster = bpb->bpbSecPerClust;
    // Find the sector for FAT#1.  It starts past the reserved sectors.
    fat_fs->first_fat_sector = bpb->bpbResSectors + first_sector;

    // FirstDirSector is only used for FAT16
    fat_fs->first_dir_sector = bpb->bpbResSectors
        + bpb->bpbFATs * bpb->bpbFATsecs + first_sector;

    if (fat_fs->isFAT32)
        fat_fs->root_dir_cluster = bpb->bpbRootClust;
    else        
        fat_fs->root_dir_cluster = 0; // special case

    fat_fs->current_dir_cluster = fat_fs->root_dir_cluster;

    fat_fs->bytes_per_cluster 
        = fat_fs->sectors_per_cluster * fat_fs->bytes_per_sector;

    TRACE_INFO (FAT, "FAT:Bytes/sector = %u\n", (unsigned int)fat_fs->bytes_per_sector);
    TRACE_INFO (FAT, "FAT:First sector = %u\n",
                (unsigned int)first_sector);
    TRACE_INFO (FAT, "FAT:Sectors/cluster = %u\n",
                (unsigned int)fat_fs->sectors_per_cluster);
    TRACE_INFO (FAT, "FAT:FirstFATSector = %u\n", 
                (unsigned int)fat_fs->first_fat_sector);
    TRACE_INFO (FAT, "FAT:FirstDataSector = %u\n",
                (unsigned int)fat_fs->first_data_sector);
    TRACE_INFO (FAT, "FAT:FirstDirSector = %u\n", 
                (unsigned int)fat_fs->first_dir_sector);
    TRACE_INFO (FAT, "FAT:RootDirCluster = %u\n",
                (unsigned int)fat_fs->root_dir_cluster);

    return fat_fs;
}


static void
fat_cluster_next_set (fat_fs_t *fat_fs __unused__, uint32_t cluster __unused__)
{
#if 0
    /* Need to read/write fsinfo.  */
    fat_sector_cache_read (fat_fs, fat_fs->first_fat_sector);
    /* Set entry at index 492.  */
    fat_sector_cache_write (fat_fs, fat_fs->first_fat_sector);
#endif
}


#if 0
static uint32_t
fat_cluster_next_get (fat_fs_t *fat_fs __unused__)
{
#if 0
    /* Need to read/write fsinfo.  */
    fat_sector_cache_read (fat_fs, fat_fs->first_fat_sector);
    /* Get entry at index 492.  */
#endif
    return CLUST_EOFE;
}
#endif


/* Find a free cluster by checking each chain in the FAT.  */
uint32_t
fat_cluster_free_find (fat_fs_t *fat_fs, uint32_t cluster_start)
{
    uint32_t cluster_next;
    uint32_t cluster;

    // TODO, should we update the free cluster count in fsinfo?
    // Set the new free cluster hint to unknown
    fat_cluster_next_set (fat_fs, CLUST_EOFE);

    for (cluster = cluster_start; cluster < fat_fs->num_clusters; cluster++)
    {
        cluster_next = fat_cluster_next (fat_fs, cluster);
        if (!cluster_next)
            return cluster;
    }
    return 0;
} 


static uint32_t
fat_cluster_chain_append (fat_fs_t *fat_fs, uint32_t cluster_start,
                          uint32_t cluster_new)
{
    if (!fat_cluster_last_p (fat_entry_get (fat_fs, cluster_start)))
        TRACE_ERROR (FAT, "FAT:bad chain\n");

    fat_entry_set (fat_fs, cluster_start, cluster_new);
    fat_entry_set (fat_fs, cluster_new, CLUST_EOFE);

    return cluster_new;
}


static uint32_t
fat_clusters_allocate (fat_fs_t *fat_fs, uint32_t cluster_start, uint32_t size)
{
    uint32_t num;
    uint32_t cluster_next;
    uint32_t cluster_new;

    if (!size)
        return 0;

    num = (size + fat_fs->bytes_per_cluster - 1) / fat_fs->bytes_per_cluster;

    if (!cluster_start)
    {
        cluster_new = fat_cluster_free_find (fat_fs, 2);
        if (!cluster_new)
            return 0;

        if (num == 1)
        {
            fat_entry_set (fat_fs, cluster_new, CLUST_EOFE);
            return cluster_new;
        }
        cluster_next = cluster_new;
    }
    else
        cluster_next = cluster_start;

    while (num)
    {
        cluster_new = fat_cluster_free_find (fat_fs, 2);
        if (!cluster_new)
            return 0;

        cluster_next = fat_cluster_chain_append (fat_fs, cluster_next,
                                                 cluster_new);
        if (!cluster_next)
            return 0;

        num--;
    }

    return cluster_start;
}

static void
fat_cluster_chain_free (fat_fs_t *fat_fs, uint32_t cluster_start)
{
    uint32_t cluster_last;
    uint32_t cluster;
	
    // Follow a chain marking each element as free
    for (cluster = cluster_start; 
         cluster != 0 && !fat_cluster_last_p (cluster);)
    {
        cluster_last = cluster;

        cluster = fat_cluster_next (fat_fs, cluster);

        // Clear last link
        fat_entry_set (fat_fs, cluster_last, 0x00000000);
    }
} 


#if 0
static uint8_t
fat_checksum_calc (const char *filename)
{
    uint8_t checksum;
    int i;

    checksum = 0;
    for (i = 0; i < 12; i++)
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + *filename++;

    return checksum;
}
#endif


static uint8_t 
fat_filename_entries (const char *filename)
{
    return (strlen (filename) + 12) / 13;
}



// Creat short filename entry
static void
fat_de_sfn_create (fatdirentry_t *de, const char *filename, uint32_t size,
                   uint32_t cluster)
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

    str = de->extension;
    for (i = 0; i < 3; i++)
    {
        if (!*filename)
            break;
        *str++ = toupper (*filename++);
    }
    for (; i < 3; i++)
        *str++ = ' ';        

    // Set dates to 1980 unless can conjure up the date with a RTC
    de->CHundredth = 0x00;
    de->CTime[1] = de->CTime[0] = 0x00;
    de->CDate[1] = 0x00;
    de->CDate[0] = 0x20;
    de->ADate[1] = 0x00;
    de->ADate[0] = 0x20;
    de->MTime[1] = de->MTime[0] = 0x00;
    de->MDate[1] = 0x00;
    de->MDate[0] = 0x20;	
    
    de->attributes = ATTR_NORMAL;
    
    de->cluster_high = cluster >> 16;
    de->cluster_low = cluster;
    de->file_size = size;
}


bool
fat_dir_add (fat_fs_t *fat_fs, const char *filename,
             uint32_t cluster_dir, uint32_t cluster_start, 
             uint32_t size)
{
    int entries;
    fat_de_iter_t de_iter;
    fatdirentry_t *de;

    TRACE_INFO (FAT, "FAT:dir_add for %s\n", filename);

    // Iterate over direntry in current directory looking for a free slot.
    for (de = fat_de_first (fat_fs, cluster_dir, &de_iter);
         !fat_de_last_p (de); de = fat_de_next (&de_iter))
    {
        if (!fat_de_free_p (de))
            continue;
    }

    entries = fat_filename_entries (filename);
    if (entries > 1)
    {
        // TODO.  We need to allocate space in the directory
        // for the long filename
        TRACE_ERROR (FAT, "FAT:Have long filename\n");        
    }

    if (fat_de_last_p (de))
    {
        fatdirentry_t *de_next;

        de_next = fat_de_next (&de_iter);
        if (!de_next)
        {
            // TODO: Need to nab another cluster and append to directory.
            TRACE_ERROR (FAT, "FAT:Need to extend dir\n");
            return 0;
        }
        de_next->name[0] = SLOT_EMPTY;
    }

    // Create short filename entry
    fat_de_sfn_create (de, filename, size, cluster_start);

    fat_sector_cache_write (fat_fs, de_iter.sector);
    fat_sector_cache_flush (fat_fs);
    return true;
}
             

static fat_t *
fat_create (fat_t *fat, const char *pathname, uint32_t size, fat_ff_t *ff)
{
    fat_fs_t *fat_fs;
    const char *filename;

    // This routine assumes that the file does not exist
    if (fat_search (fat->fs, pathname, ff))
        return NULL;

    // Check that directory is valid
    // TODO, should we create a directory?
    if (!ff->parent_dir_cluster)
        return NULL;

    filename = pathname;
    while (strchr (filename, '/'))
        filename = strchr (filename, '/') + 1;

    // TODO, what about a trailing slash?

    fat_fs = fat->fs;

    // Create the file space for the file
    fat->file_size = size;
    fat->file_offset = 0; 

    // Create at least one cluster to start with
    fat->start_cluster =
        fat_clusters_allocate (fat_fs, 0,
                                 fat->file_size == 0 ? 1 : fat->file_size);
    if (!fat->start_cluster)
        return NULL;

    // Add file to directory
    if (!fat_dir_add (fat_fs, filename, ff->parent_dir_cluster, 
                      fat->start_cluster, fat->file_size))
        return NULL;

    fat->cluster = fat->start_cluster;
    fat_sector_cache_flush (fat_fs);

    return fat;
}


static bool
fat_fs_check_p (fat_fs_t *fat_fs)
{
    // Check for corrupt filesystem.
    return fat_fs->bytes_per_cluster != 0
        && fat_fs->bytes_per_sector != 0;
}


/**
 * Open a file
 * 
 * \param name File name
 * \param mode Mode to open file
 * 
 * - O_BINARY Raw mode.
 * - O_TEXT End of line translation. 
 * - O_EXCL Open only if it does not exist. 
 * - O_RDONLY Read only. 
    
 any of the write modes will (potentially) modify the file or directory entry

 * - O_CREAT Create file if it does not exist. 
 * - O_APPEND Always write at the end. 
 * - O_RDWR Read and write. 
 * - O_WRONLY Write only.

 * - O_TRUNC Truncate file if it exists. 
 *
 * \return File handle 
 */
fat_t *fat_open (fat_fs_t *fat_fs, const char *pathname, int mode)
{
    fat_ff_t ff;

    fat_t *fat = 0;

    if (!fat_fs_check_p (fat_fs))
    {
        errno = EFAULT;
        return 0;
    }

    TRACE_INFO (FAT, "FAT:open %s\n", pathname);

    if (!pathname || !*pathname)
        return 0;

    // Alloc for file spec
    fat = malloc (sizeof (*fat));
    if (fat == NULL)
    {
        TRACE_ERROR (FAT, "FAT:cannot alloc fat\n");
        return 0;  // fail
    }
    memset (fat, 0, sizeof (*fat));
    fat->mode = mode;
    fat->fs = fat_fs;

    if (fat_find (fat, pathname, &ff))
    {
        if (ff.isdir)
        {
            errno = EISDIR;
            return 0;
        }

        if (mode & O_TRUNC && (mode & O_RDWR || mode & O_WRONLY))
        {
            fat->file_size = 0;
            fat_size_set (fat, fat->file_size);
        }

        fat->file_offset = 0;
        if (mode & O_APPEND)
            fat->file_offset = fat->file_size;        
        return fat;
    }

    if (mode & O_CREAT)
    {
        if (fat_create (fat, pathname, 0, &ff))
        {
            fat->file_offset = 0;
            if (mode & O_APPEND)
                fat->file_offset = fat->file_size;        
            return fat;
        }
        TRACE_INFO (FAT, "FAT:file not created: %s\n", pathname);
    }
    else
    {
        TRACE_INFO (FAT, "FAT:file not found: %s\n", pathname);
    }

    free (fat);
    return 0;
}


ssize_t
fat_write (fat_t *fat, const void *buffer, size_t len)
{
    uint16_t nbytes;
    uint32_t sector;
    uint16_t bytes_left;
    uint16_t offset;
    const uint8_t *data;

    TRACE_INFO (FAT, "FAT:write %u\n", (unsigned int)len);

    if (! ((fat->mode & O_RDWR) || (fat->mode & O_WRONLY)))
    {
        errno = EINVAL;
        return -1;
    }

    data = buffer;
    bytes_left = len;
    while (bytes_left)
    {
        offset = fat->file_offset % fat->fs->bytes_per_sector;
        sector = fat_sector_calc (fat->fs, fat->cluster);

        sector += (fat->file_offset % fat->fs->bytes_per_cluster) 
            / fat->fs->bytes_per_sector;

        // Limit to max one sector
        nbytes = bytes_left < fat->fs->bytes_per_sector
            ? bytes_left : fat->fs->bytes_per_sector;

        // Limit to remaining bytes in a sector
        if (nbytes > (fat->fs->bytes_per_sector - offset))
            nbytes = fat->fs->bytes_per_sector - offset;

        fat_partial_sector_write (fat->fs, sector, offset, data, nbytes);

        data += nbytes;

        fat->file_offset += nbytes;
        bytes_left -= nbytes;

        // Check for cluster boundary
        if (fat->file_offset % fat->fs->bytes_per_cluster == 0)
        {
            // Append a new cluster
            fat->cluster = fat_clusters_allocate (fat->fs, fat->cluster, 1);

            // No more clusters to allocate, out of memory
            if (!fat->cluster)
                break;
        }
    }
    fat->file_size += len - bytes_left;

    fat_size_set (fat, fat->file_size);
    fat_sector_cache_flush (fat->fs);

    TRACE_INFO (FAT, "FAT:write return %u\n", (unsigned int)len - bytes_left);
    return len - bytes_left;
}


int
fat_unlink (fat_fs_t *fat_fs, const char *pathname)
{
    fat_ff_t ff;
    fat_de_iter_t de_iter;
    fatdirentry_t *de;

    TRACE_INFO (FAT, "FAT:unlink %s\n", pathname);

    // Should figure out if the file is open and return EBUSY
    // Would need to maintain a table of open files

    if (!fat_search (fat_fs, pathname, &ff))
    {
        errno = ENOENT;
        return -1;
    }

    if (ff.isdir)
    {
        // TODO, Need to scan the directory and check it is empty
        // For now just punt
        errno = EISDIR;
        return -1;
    }

    fat_cluster_chain_free (fat_fs, ff.cluster);

    for (de = fat_de_first (fat_fs, ff.parent_dir_cluster, &de_iter);
         !fat_de_last_p (de); de = fat_de_next (&de_iter))
    {
        /* Find start of dir entry.  */
        if (de_iter.offset == ff.dir_offset && de_iter.sector == ff.dir_sector)
        {
            for (; fat_de_attr_long_filename_p (de);
                 de = fat_de_next (&de_iter))
                de->name[0] = SLOT_DELETED;                
            
            de->name[0] = SLOT_DELETED;                
            fat_sector_cache_flush (fat_fs);
            return 0;
        }
    }

    TRACE_ERROR (FAT, "FAT:unlink lost dir entry\n");
    return 0;
}

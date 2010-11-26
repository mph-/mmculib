/** @file   fat.c
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT filesystem routines for manipulating the FAT.
*/

#include "fat.h"
#include "fat_de.h"
#include "fat_io.h"
#include "fat_fsinfo.h"
#include "fat_cluster.h"


/*
   For a simplified description of FAT32 see 
   http://www.pjrc.com/tech/8051/ide/fat32.html
*/

#define CLUST_FIRST     2



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


/** @struct partrecord 
 * Partition Record Structure
 * 
 */ 
struct partrecord
{           
    uint8_t     prIsActive;            //!< 0x80 indicates active partition 
    uint8_t     prStartHead;           //!< Starting head for partition 
    uint16_t    prStartCylSect;        //!< Starting cylinder and sector 
    uint8_t     prPartType;            //!< Partition type (see above) 
    uint8_t     prEndHead;             //!< Ending head for this partition 
    uint16_t    prEndCylSect;          //!< Ending cylinder and sector 
    uint32_t    prStartLBA;            //!< First LBA sector for this partition 
    uint32_t    prSize;                //!< Size of this partition
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


/** @struct partsector
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


/** @struct bpb710 
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
#define FATMIRROR       0x80            //!< FAT is mirrored
    uint16_t        bpbFSVers;      //!< Filesystem version
#define FSVERS          0               //!< Currently only 0 is understood
    uint32_t        bpbRootClust;   //!< Start cluster for root directory
    uint16_t        bpbFSInfo;      //!< Filesystem info structure sector
    uint16_t        bpbBackup;      //!< Backup boot sector 
    uint8_t         bpbFiller[12];  //!< Reserved
} __packed__;


/** @struct extboot
 * Fat12 and Fat16 boot block structure
 * 
 */ 
struct extboot
{
    uint8_t         exDriveNumber;        //!< Drive number (0x80)
    uint8_t         exReserved1;          //!< Reserved
    uint8_t         exBootSignature;      //!< Ext. boot signature (0x29)
#define EXBOOTSIG       0x29                //!< Extended boot signature                
    uint8_t         exVolumeID[4];        //!< Volume ID number
    uint8_t         exVolumeLabel[11];    //!< Volume label
    uint8_t         exFileSysType[8];     //!< File system type (FAT12 or FAT16)
} __packed__;


/** @struct bootsector710 
 * Boot Sector.
 * This is the first sector on a DOS floppy disk or the first sector
 * of a partition on a hard disk.  But, it is not the first sector of
 * a partitioned hard disk.
 */
struct bootsector710 
{
    uint8_t         bsJump[3];          //!< Jump inst E9xxxx or EBxx90 
    char            bsOEMName[8];       //!< OEM name and version 
    struct bpb710   bsBPB;              //!< BPB block
    struct extboot  bsExt;              //!< Bootsector extension 

    uint8_t         bsBootCode[418];    //!< Pad so structure size is 512 bytes
    uint8_t         bsBootSectSig2;     //!< 2 & 3 are only defined for FAT32? 
    uint8_t         bsBootSectSig3;     //!< 2 & 3 are only defined for FAT32?
    uint8_t         bsBootSectSig0;     //!< 
    uint8_t         bsBootSectSig1;     //!<

#define BOOTSIG0        0x55            //!< Signature constant 0
#define BOOTSIG1        0xaa            //!< Signature constant 1
#define BOOTSIG2        0               //!< Signature constant 2
#define BOOTSIG3        0               //!< Signature constant 3
} __packed__;



/* Number of concurrent FAT file systems that can be supported.  */
#ifndef FAT_FS_NUM
#define FAT_FS_NUM 1
#endif


static fat_t fat_info[FAT_FS_NUM];
static uint8_t fat_num;



/** Return number of sectors for a directory.  */
int
fat_dir_sector_count (fat_t *fat, uint32_t cluster)
{
    if (fat->type == FAT_FAT16 && cluster == fat->root_dir_cluster)
        return fat->root_dir_sectors; 
    else
        return fat->sectors_per_cluster;
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


void
fat_dir_dump (fat_t *fat, uint32_t dir_cluster)
{
    fat_de_dir_dump (fat, dir_cluster);
}


void
fat_rootdir_dump (fat_t *fat)
{
    fat_dir_dump (fat, fat->root_dir_cluster);
}


bool
fat_check_p (fat_t *fat)
{
    /* Check for corrupt filesystem.  */
    return fat->bytes_per_cluster != 0
        && fat->bytes_per_sector != 0;
}



uint16_t
fat_sector_size (fat_t *fat)
{
    return fat->bytes_per_sector;
}



uint16_t
fat_root_dir_cluster (fat_t *fat)
{
    return fat->root_dir_cluster;
}


/**
 * Init FAT
 * 
 * Initialise things by reading partition information.
 * 
 */
fat_t *
fat_init (void *dev, fat_dev_read_t dev_read, fat_dev_write_t dev_write)
{
    uint32_t tot_sectors;
    uint32_t data_sectors;
    struct partrecord *pr = NULL;
    struct bootsector710 *pb;
    struct bpb710 *bpb;
    struct extboot *bsext;
    fat_t *fat;

    fat = &fat_info[fat_num++];
    if (fat_num > FAT_FS_NUM)
        return NULL;

    fat->dev = dev;
    fat->dev_read = dev_read;
    fat->dev_write = dev_write;

    fat_io_cache_init (fat);

    TRACE_INFO (FAT, "FAT:Init\n");

    /* Read first sector on device.  .  */
    fat->bytes_per_sector = FAT_SECTOR_SIZE;
    fat_io_cache_read (fat, 0);

    /* Check for a jump instruction marking start of MBR.  */
    if (*fat->cache.buffer == 0xE9 || *fat->cache.buffer == 0xEB)
    {
        /* Have a boot sector but no partition sector.  */
        fat->first_sector = 0;
        TRACE_ERROR (FAT, "FAT:Found MBR, fixme\n");
        return NULL;
    }
    else
    {
        struct partsector *ps;

        /* Get the first partition record from the data.  */
        ps = (struct partsector *) fat->cache.buffer;
        pr = (struct partrecord *) ps->psPart;
        /* Get partition start sector.  */
        fat->first_sector = pr->prStartLBA;
    }

    /** @todo Add more FAT16 types here.  Note mkdosfs needs -N32
        option to use 32 bits for each FAT otherwise it will default
        to 12 for small systems and this code is not robust enought to
        handle this.  */
    switch (pr->prPartType)
    {
    case PART_TYPE_FAT16:
        TRACE_INFO (FAT, "FAT:FAT16\n");
        fat->type = FAT_FAT16;
        break;
        
    case PART_TYPE_FAT32:
    case PART_TYPE_FAT32LBA:
        TRACE_INFO (FAT, "FAT:FAT32\n");
        fat->type = FAT_FAT32;
        break;

    default:
        TRACE_INFO (FAT, "FAT:Unknown\n");

        /* Most likely a file system hasn't been created.  */
        return NULL;
    }
    
    /* Read partition boot record (Volume ID).  */
    fat_io_cache_read (fat, fat->first_sector); 

    /* Point to partition boot record.  */
    pb = (struct bootsector710 *) fat->cache.buffer;
    bpb = &pb->bsBPB;
    bsext = &pb->bsExt;
    TRACE_INFO (FAT, "FAT:%s\n", bsext->exVolumeLabel);

    /* Number of bytes per sector.  */
    fat->bytes_per_sector = le16_to_cpu (bpb->bpbBytesPerSec);

    /* File system info sector.  */
    fat->fsinfo_sector = le16_to_cpu (bpb->bpbFSInfo);
    if (!fat->fsinfo_sector)
        fat->fsinfo_sector = 1;
    fat->fsinfo_sector += fat->first_sector;
    fat->fsinfo_dirty = 0;

    /* Number of sectors per FAT.  */
    fat->num_fat_sectors = le16_to_cpu (bpb->bpbFATsecs);
    if (!fat->num_fat_sectors)
        fat->num_fat_sectors = le32_to_cpu (bpb->bpbBigFATsecs);

    /* Number of sectors in root dir (will be 0 for FAT32).  */
    fat->root_dir_sectors  
        = ((le16_to_cpu (bpb->bpbRootDirEnts) * 32)
           + (fat->bytes_per_sector - 1)) / fat->bytes_per_sector;

    /* First data sector on the volume.  */
    fat->first_data_sector = bpb->bpbResSectors + bpb->bpbFATs
        * fat->num_fat_sectors + fat->root_dir_sectors;

    /* Calculate total number of sectors on the volume.  */
    tot_sectors = le16_to_cpu (bpb->bpbSectors);
    if (!tot_sectors)
        tot_sectors = le32_to_cpu (bpb->bpbHugeSectors);
        
    /* Total number of data sectors.  */
    data_sectors = tot_sectors - fat->first_data_sector;

    /* Total number of clusters.  */
    fat->num_clusters = data_sectors / bpb->bpbSecPerClust;

    fat->first_data_sector += fat->first_sector;
    fat->sectors_per_cluster = bpb->bpbSecPerClust;
    /* Find the sector for FAT1.  It starts past the reserved sectors.  */
    fat->first_fat_sector = bpb->bpbResSectors + fat->first_sector;

    /* FAT2 (if it exists) comes after FAT1.  */

    fat->first_dir_sector = bpb->bpbResSectors
        + bpb->bpbFATs * fat->num_fat_sectors + fat->first_sector;

    if (fat->type == FAT_FAT32)
        fat->root_dir_cluster = le32_to_cpu (bpb->bpbRootClust);
    else        
        fat->root_dir_cluster = 0;

    fat->bytes_per_cluster 
        = fat->sectors_per_cluster * fat->bytes_per_sector;

    TRACE_INFO (FAT, "FAT:Data sectors = %ld\n", data_sectors);
    TRACE_INFO (FAT, "FAT:Clusters = %ld\n", fat->num_clusters);
    TRACE_INFO (FAT, "FAT:Bytes/sector = %u\n",
                (unsigned int)fat->bytes_per_sector);
    TRACE_INFO (FAT, "FAT:Sectors/cluster = %u\n",
                (unsigned int)fat->sectors_per_cluster);
    TRACE_INFO (FAT, "FAT:First sector = %u\n",
                (unsigned int)fat->first_sector);
    TRACE_INFO (FAT, "FAT:FirstFATSector = %u\n", 
                (unsigned int)fat->first_fat_sector);
    TRACE_INFO (FAT, "FAT:FirstDataSector = %u\n",
                (unsigned int)fat->first_data_sector);
    TRACE_INFO (FAT, "FAT:FirstDirSector = %u\n", 
                (unsigned int)fat->first_dir_sector);
    TRACE_INFO (FAT, "FAT:RootDirCluster = %u\n",
                (unsigned int)fat->root_dir_cluster);

    return fat;
}

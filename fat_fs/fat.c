/** @file   fat.c
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT filesystem routines for manipulating the FAT.
*/

#include "fat.h"
#include "fat_de.h"
#include "fat_io.h"

/*
   For a simplified description of FAT32 see 
   http://www.pjrc.com/tech/8051/ide/fat32.html
*/


#include "fat_common.h"


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


/** @struct fsinfo
 *  FAT32 FSInfo block (note this spans several sectors).
 */
struct fsinfo
{
    uint32_t fsisig1;           //!< 0x41615252
    uint8_t fsifill1[480];      //!< Reserved
    uint32_t fsisig2;           //!< 0x61417272
    uint32_t fsinfree;          //!< Last known free cluster count
    uint32_t fsinxtfree;        //!< Last free cluster
    uint8_t fsifill2[12];       //!< Reserved 
    uint8_t fsisig3[4];         //!< Trail signature
    uint8_t fsifill3[508];      //!< Reserved
    uint8_t fsisig4[4];         //!< Sector signature 0xAA55
} __packed__;


#define FAT_FSINFO_SIG1	0x41615252
#define FAT_FSINFO_SIG2	0x61417272
#define FAT_FSINFO_P(x)	(le32_to_cpu ((x)->signature1) == FAT_FSINFO_SIG1 \
			 && le32_to_cpu ((x)->signature2) == FAT_FSINFO_SIG2)



/* Statistics structure.  */
typedef struct fat_stats_struct
{
    unsigned int total;
    unsigned int free;
    unsigned int alloc;
} fat_stats_t;



/* Number of concurrent FAT file systems that can be supported.  */
#ifndef FAT_FS_NUM
#define FAT_FS_NUM 1
#endif


static fat_t fat_info[FAT_FS_NUM];
static uint8_t fat_num;


/* Return true if cluster is free.  */
static inline bool
fat_cluster_free_p (uint32_t cluster)
{
    return cluster == CLUST_FREE;
}


/**
 * Convert a cluster number to a sector number.
 * @param cluster Cluster number
 * @return Sector number
 */
uint32_t
fat_sector_calc (fat_t *fat, uint32_t cluster)
{
    /* If this is a cluster request for the rootdir, point to it.  */
    if (cluster == 0)
        return fat->first_dir_sector;

    /* Clusters are numbered starting from CLUST_FIRST.  */
    return ((uint32_t) (cluster - CLUST_FIRST) * fat->sectors_per_cluster) 
        + fat->first_data_sector;    
}


/**
 * Get FAT entry
 * 
 * @param   cluster Entry in FAT to look up
 * @return  Next cluster in chain
 */
static uint32_t 
fat_entry_get (fat_t *fat, uint32_t cluster)
{
    uint32_t sector, offset, cluster_new, mask;
    
    /* Calculate the sector number and sector offset in the FAT for
       this cluster number.  */
    
    if (fat->type == FAT_FAT32)
    {
        /* There are 4 bytes per FAT entry in FAT32.  */
        offset = cluster << 2;
        mask = FAT32_MASK;
    }
    else
    {
        /* There are 2 bytes per FAT entry in FAT16.  */
        offset = cluster << 1;
        mask = FAT16_MASK;
    }
    
    /* Read sector of FAT1 for desired cluster entry.  */
    sector = fat->first_fat_sector + offset / fat->bytes_per_sector;
    fat_io_cache_read (fat, sector);
    
    /* Get the data for desired FAT entry.  */
    offset = offset % fat->bytes_per_sector;
    if (fat->type == FAT_FAT32)
        cluster_new = le32_get (fat->cache.buffer + offset);
    else
        cluster_new = le16_get (fat->cache.buffer + offset);

    /* A value of zero in the FAT indicates a free cluster.  A value
       greater than or equal to 0xFFFFFFF8 marks the end of a chain.  */

    if (cluster_new >= (CLUST_EOFS & mask))
        return CLUST_EOFS;

    return cluster_new & mask;
}


/* Return the FAT entry checking that it is valid and not free.  */
uint32_t 
fat_cluster_next (fat_t *fat, uint32_t cluster)
{
    uint32_t cluster_new;

   cluster_new = fat_entry_get (fat, cluster);

   if (fat_cluster_free_p (cluster_new))
   {
       TRACE_ERROR (FAT, "FAT:Entry %u free\n", (unsigned int) cluster);
       return CLUST_EOFE;
   }
   return cluster_new;
}


/* Set a FAT entry.  */
static void
fat_entry_set (fat_t *fat, uint32_t cluster, uint32_t cluster_new)
{
    uint32_t sector, offset;
    
    /* Calculate the sector number and sector offset in the FAT for
       this cluster number.  */
    
    if (fat->type == FAT_FAT32)
    {
        /* There are 4 bytes per FAT entry in FAT32.  */
        offset = cluster << 2;
    }
    else
    {
        /* There are 2 bytes per FAT entry in FAT16.  */
        offset = cluster << 1;
    }

    /* Read sector of FAT for desired cluster entry.  */
    sector = fat->first_fat_sector + offset / fat->bytes_per_sector;
    fat_io_cache_read (fat, sector);

    /* Set the data for desired FAT entry.  */
    offset = offset % fat->bytes_per_sector;

    if (fat->type == FAT_FAT32)
    {
        le32_set (fat->cache.buffer + offset, cluster_new);
    }
    else
    {
        le16_set (fat->cache.buffer + offset, cluster_new);        
    }

    fat_io_cache_write (fat, sector);
}


/** Return number of sectors for a directory.  */
int
fat_dir_sector_count (fat_t *fat, uint32_t cluster)
{
    if (fat->type == FAT_FAT16 && cluster == fat->root_dir_cluster)
        return fat->root_dir_sectors; 
    else
        return fat->sectors_per_cluster;
}


uint16_t 
fat_chain_length (fat_t *fat, uint32_t cluster)
{
    uint16_t length;

    if (!cluster)
        return 0;

    length = 0;
    while (1)
    {
        length++;
        cluster = fat_cluster_next (fat, cluster);
        if (fat_cluster_last_p (cluster))
            break;
    }
    return length;
}


/**
 * Init FAT
 * 
 * Initialise things by reading file system information.
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


void
fat_fsinfo_read (fat_t *fat)
{
    struct fsinfo *fsinfo = (void *)fat->cache.buffer;

    fat_io_cache_read (fat, fat->fsinfo_sector);
    /* Should check signatures.  */

    /* These fields are not necessarily correct.  */
    fat->free_clusters = le32_to_cpu (fsinfo->fsinfree);
    if (fat->free_clusters > fat->num_clusters)
        fat->free_clusters = ~0u;
    
    fat->prev_free_cluster = le32_to_cpu (fsinfo->fsinxtfree);
    if (fat->prev_free_cluster < CLUST_FIRST
        || fat->prev_free_cluster >= fat->num_clusters)
        fat->prev_free_cluster = CLUST_FIRST;

    fat->fsinfo_dirty = 0;
}


static void
fat_fsinfo_write (fat_t *fat)
{
    struct fsinfo *fsinfo = (void *)fat->cache.buffer;

    if (!fat->fsinfo_dirty)
        return;

    fat_io_cache_read (fat, fat->fsinfo_sector);
    fsinfo->fsinfree = cpu_to_le32 (fat->free_clusters);
    fsinfo->fsinxtfree = cpu_to_le32 (fat->prev_free_cluster);
    fat_io_cache_write (fat, fat->fsinfo_sector);
}


static void
fat_free_clusters_update (fat_t *fat, int count)
{
    /* Do nothing if free clusters invalid.  */
    if (fat->free_clusters == ~0u)
        return;

    fat->free_clusters += count;
    fat->fsinfo_dirty = 1;
}


static void
fat_prev_free_cluster_set (fat_t *fat, uint32_t cluster)
{
    fat->prev_free_cluster = cluster;
    fat->fsinfo_dirty = 1;
}


uint32_t
fat_cluster_free_search (fat_t *fat, uint32_t start, uint32_t stop)
{
    uint32_t cluster;

    /* Linearly search through the FAT looking for a free cluster.  */
    for (cluster = start; cluster < stop; cluster++)
    {
        if (fat_cluster_free_p (fat_entry_get (fat, cluster)))
            return cluster;
    }
    
    return 0;
}


/* Find a free cluster by checking each chain in the FAT.  */
uint32_t
fat_cluster_free_find (fat_t *fat)
{
    uint32_t cluster;
    uint32_t cluster_start;

    cluster_start = fat->prev_free_cluster + 1;

    cluster = fat_cluster_free_search (fat, cluster_start, 
                                       fat->num_clusters);
    if (!cluster)
        cluster = fat_cluster_free_search (fat, CLUST_FIRST, 
                                           cluster_start);        

    /* Out of memory.  */
    if (!cluster)
        return 0;

    fat_free_clusters_update (fat, -1);
    fat_prev_free_cluster_set (fat, cluster);
    return cluster;
}


static uint32_t
fat_cluster_chain_append (fat_t *fat, uint32_t cluster_start,
                          uint32_t cluster_new)
{
    if (!fat_cluster_last_p (fat_entry_get (fat, cluster_start)))
        TRACE_ERROR (FAT, "FAT:Bad chain\n");

    fat_entry_set (fat, cluster_start, cluster_new);

    return cluster_new;
}


void
fat_cluster_chain_free (fat_t *fat, uint32_t cluster_start)
{
    uint32_t cluster_last;
    uint32_t cluster;
    int count;
	
    /* Follow a chain marking each element as free.  */
    count = 0;
    for (cluster = cluster_start; !fat_cluster_last_p (cluster);)
    {
        cluster_last = cluster;

        cluster = fat_cluster_next (fat, cluster);

        /* Mark cluster as free.  */
        fat_entry_set (fat, cluster_last, 0x00000000);
        count++;
    }
    fat_free_clusters_update (fat, count);

    fat_fsinfo_write (fat);
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


/* Return new cluster or zero if out of memory.  */
static uint32_t
fat_cluster_allocate (fat_t *fat, uint32_t cluster_start)
{
    uint32_t cluster;

    /* Find free cluster.  */
    cluster = fat_cluster_free_find (fat);

    /* Check if have run out of memory.  */
    if (!cluster)
        return 0;

    /* Mark cluster as the end of a chain.  */
    fat_entry_set (fat, cluster, CLUST_EOFE);    

    /* Append to cluster chain.  */
    if (cluster_start)
        fat_cluster_chain_append (fat, cluster_start, cluster);

    return cluster;
}


uint32_t
fat_chain_extend (fat_t *fat, uint32_t cluster_start, 
                  uint32_t num_clusters)
{
    uint32_t first_cluster;

    fat_fsinfo_read (fat);

    /* Walk to end of current chain.  */
    if (cluster_start)
    {
        while (1)
        {
            uint32_t cluster;

            cluster = fat_cluster_next (fat, cluster_start);
            if (fat_cluster_last_p (cluster))
                break;
            cluster_start = cluster;
        }
    }

    first_cluster = fat_cluster_allocate (fat, cluster_start);
    cluster_start = first_cluster;
    while (--num_clusters)
    {
        cluster_start = fat_cluster_allocate (fat, cluster_start);
    }

    fat_fsinfo_write (fat);

    return first_cluster;
}


void
fat_stats (fat_t *fat, fat_stats_t *stats)
{
    uint32_t cluster;
    
    stats->alloc = 0;

    /* Iterate over all clusters counting allocated clusters.  */
    for (cluster = CLUST_FIRST; cluster < fat->num_clusters; cluster++)
    {
        if (!fat_cluster_free_p (fat_entry_get (fat, cluster)))
            stats->alloc++;
    }

    stats->total = fat->num_clusters;
    stats->free = stats->total - stats->alloc;
}


void
fat_stats_dump (fat_t *fat)
{
    fat_stats_t stats;

    fat_stats (fat, &stats);
    
    TRACE_ERROR (FAT, "Free  %u\n", stats.free);
    TRACE_ERROR (FAT, "Alloc %u\n", stats.alloc);
    TRACE_ERROR (FAT, "Total %u\n", stats.total);
}


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


void
fat_fsinfo_fix (fat_t *fat)
{
    uint32_t cluster;
    bool found;

    fat->free_clusters = 0;
    found = 0;

    /* Iterate over all clusters.  */
    for (cluster = CLUST_FIRST; cluster < fat->num_clusters; cluster++)
    {
        if (fat_cluster_free_p (fat_entry_get (fat, cluster)))
        {
            fat->free_clusters++;
            if (!found)
            {
                /* This is not quite correct; it should be the last
                   allocated cluster.  */
                fat->prev_free_cluster = cluster;
                found = 1;
            }
        }
    }
    fat->fsinfo_dirty = 1;
    fat_fsinfo_write (fat);
    fat_io_cache_flush (fat);
    fat->fsinfo_dirty = 0;
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

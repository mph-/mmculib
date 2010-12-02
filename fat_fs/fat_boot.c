#include <string.h>
#include "fat_boot.h"
#include "fat_io.h"

/** @struct bpb710 
 * BPB for DOS 7.10 (FAT32).  This is an extended version of bpb50.
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
    char            exFileSysType[8];     //!< File system type (FAT12 or FAT16)
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


/** Read boot record.  */
bool
fat_boot_read (fat_t *fat)
{
    uint8_t *buffer;
    struct bootsector710 *pb;
    struct bpb710 *bpb;
    struct extboot *bsext;
    uint32_t tot_sectors;
    uint32_t data_sectors;
    char label[12];
    int i;

    /* Read partition boot record (Volume ID).  */
    buffer = fat_io_cache_read (fat, fat->first_sector); 
    if (!buffer)
        return 0;

    /* Point to partition boot record.  */
    pb = (struct bootsector710 *) buffer;
    bpb = &pb->bsBPB;
    bsext = &pb->bsExt;

    for (i = 0; i < 11; i++)
        label[i] = bsext->exVolumeLabel[i];
    label[i] = 0;
    TRACE_INFO (FAT, "FAT:%s\n", label);

    if (fat->type == FAT_UNKNOWN)
    {
        if (strncmp (bsext->exFileSysType, "FAT32", 5) == 0)
            fat->type = FAT_FAT32;
        else if (strncmp (bsext->exFileSysType, "FAT16", 5) == 0)
            fat->type = FAT_FAT16;
    }

    /* Number of bytes per sector.  */
    fat->bytes_per_sector = le16_to_cpu (bpb->bpbBytesPerSec);

    /* File system info sector.  */
    fat->fsinfo_sector = le16_to_cpu (bpb->bpbFSInfo);
    if (!fat->fsinfo_sector)
        fat->fsinfo_sector = 1;
    fat->fsinfo_sector += fat->first_sector;

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

    return 1;
}



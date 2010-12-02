/** @file   fat.c
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT filesystem routines for reading the partition record.
*/

#include "fat.h"
#include "fat_io.h"
#include "fat_boot.h"
#include "fat_fsinfo.h"


/*
   For a simplified description of FAT32 see 
   http://www.pjrc.com/tech/8051/ide/fat32.html
*/


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


bool
fat_partition_read (fat_t *fat)
{
    uint8_t *buffer;
    struct partrecord *pr;
    struct partsector *ps;


    /* Read first sector on device.  */
    fat->bytes_per_sector = FAT_SECTOR_SIZE;
    fat->type = FAT_UNKNOWN;

    buffer = fat_io_cache_read (fat, 0);

    /* Check for a jump instruction marking start of MBR.  */
    if (*buffer == 0xE9 || *buffer == 0xEB)
    {
        /* Have a boot sector but no partition sector.  Thus there is only
         a single partition (like with a floppy disk).*/
        fat->first_sector = 0;
        TRACE_ERROR (FAT, "FAT:Found MBR\n");
    }
    else
    {
        ps = (struct partsector *) buffer;
        
        /* Get the first partition record.  */
        pr = (struct partrecord *) ps->psPart;
        
        /* Get partition start sector.  */
        fat->first_sector = le32_to_cpu (pr->prStartLBA);
        
        /** @todo Add more FAT16 types here.  Note mkdosfs needs -N32
            option to use 32 bits for each FAT otherwise it will default
            to 12 for small systems and this code is not robust enough to
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
            return 0;
        }
    }

    if (!fat_boot_read (fat))
        return 0;

    if (!fat_fsinfo_read (fat))
        return 0;
    
    return 1;
}


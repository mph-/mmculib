/** @file   fat_fsinfo.c
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT filesystem routines for the file system info.
*/

#include "fat.h"
#include "fat_cluster.h"
#include "fat_io.h"


/* This implements handling of the fields in the file system info
   (fsinfo) sectors, in particular, the number of free clusters and
   the last free cluster.  These fields are used as hints so do not
   have to be accurate.  They are read by fat_fsinfo_read (this only
   needs to be performed once) and are cached.  They are written by
   fat_fsinfo_write.  
*/


#define CLUST_FIRST     2


/** @struct fsinfo
 *  FAT32 FSInfo block (note this spans two 512 byte sectors although
 *  the interesting stuff is in the first sector).
 */
struct fsinfo
{
    uint32_t fsisig1;           //!< 0x41615252
    uint8_t fsifill1[480];      //!< Reserved
    uint32_t fsisig2;           //!< 0x61417272
    uint32_t fsinfree;          //!< Last known free cluster count
    uint32_t fsinxtfree;        //!< Last free cluster
    uint8_t fsifill2[12];       //!< Reserved 
    uint8_t fsisig3[4];         //!< Trailing signature

    uint8_t fsifill3[508];      //!< Reserved
    uint8_t fsisig4[4];         //!< Sector signature 0xAA55
} __packed__;




#define FAT_FSINFO_SIG1	0x41615252
#define FAT_FSINFO_SIG2	0x61417272
#define FAT_FSINFO_P(x)	(le32_to_cpu ((x)->signature1) == FAT_FSINFO_SIG1 \
			 && le32_to_cpu ((x)->signature2) == FAT_FSINFO_SIG2)



void
fat_fsinfo_free_clusters_set (fat_t *fat, int count)
{
    fat->free_clusters = count;
    fat->fsinfo_dirty = 1;
}


void
fat_fsinfo_free_clusters_update (fat_t *fat, int count)
{
    /* Do nothing if free clusters invalid.  */
    if (fat->free_clusters == ~0u)
        return;

    fat->free_clusters += count;
    fat->fsinfo_dirty = 1;
}


void
fat_fsinfo_prev_free_cluster_set (fat_t *fat, uint32_t cluster)
{
    fat->prev_free_cluster = cluster;
    fat->fsinfo_dirty = 1;
}


uint32_t
fat_fsinfo_prev_free_cluster_get (fat_t *fat)
{
    return fat->prev_free_cluster;
}


uint32_t
fat_fsinfo_free_clusters_get (fat_t *fat)
{
    return fat->free_clusters;
}


bool
fat_fsinfo_read (fat_t *fat)
{
    struct fsinfo *fsinfo;

    /* Read the first sector of the fsinfo.  */
    fsinfo = (void *)fat_io_cache_read (fat, fat->fsinfo_sector);
    if (!fsinfo)
        return 0;

    /* TODO: Should check signatures.  */

    /* These fields are not necessarily correct but try our best.  */
    fat->free_clusters = le32_to_cpu (fsinfo->fsinfree);
    if (fat->free_clusters > fat->num_clusters)
        fat->free_clusters = ~0u;
    
    fat->prev_free_cluster = le32_to_cpu (fsinfo->fsinxtfree);
    if (fat->prev_free_cluster < CLUST_FIRST
        || fat->prev_free_cluster >= fat->num_clusters)
        fat->prev_free_cluster = CLUST_FIRST;

    fat->fsinfo_dirty = 0;
    return 1;
}


void
fat_fsinfo_write (fat_t *fat)
{
    struct fsinfo *fsinfo;

    if (!fat->fsinfo_dirty)
        return;

    fsinfo = (void *)fat_io_cache_read (fat, fat->fsinfo_sector);
    fsinfo->fsinfree = cpu_to_le32 (fat->free_clusters);
    fsinfo->fsinxtfree = cpu_to_le32 (fat->prev_free_cluster);
    fat_io_cache_write (fat, fat->fsinfo_sector);

    /* Ensure the cached sector is written.  */
    fat_io_cache_flush (fat);

    fat->fsinfo_dirty = 0;
}


void
fat_fsinfo_fix (fat_t *fat)
{
    fat_cluster_stats_t stats;

    fat_cluster_stats (fat, &stats);

    fat_fsinfo_prev_free_cluster_set (fat, stats.prev_free_cluster);
    fat_fsinfo_free_clusters_set (fat, stats.free);

    fat_fsinfo_write (fat);
}



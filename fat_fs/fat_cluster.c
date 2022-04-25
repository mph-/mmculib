/** @file   fat_cluster.c
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem FAT cluster fmanipulation routines.
*/


#include <inttypes.h>

#include "fat.h"
#include "fat_fsinfo.h"
#include "fat_cluster.h"
#include "fat_io.h"

#define CLUST_FREE      0               //!< Cluster 0 also means a free cluster
#define CLUST_FIRST     2               //!< First legal cluster number 
#define CLUST_RSRVD     0xfffffff6u     //!< Reserved cluster range 
#define CLUST_BAD       0xfffffff7u     //!< A cluster with a defect 
#define CLUST_EOFS      0xfffffff8u     //!< Start of eof cluster range 
#define CLUST_EOFE      0xffffffffu     //!< End of eof cluster range 


#define FAT12_MASK      0x00000fff      //!< Mask for 12 bit cluster numbers 
#define FAT16_MASK      0x0000ffff      //!< Mask for 16 bit cluster numbers 
#define FAT32_MASK      0x0fffffff      //!< Mask for FAT32 cluster numbers 



/* Return true if cluster is free.  */
bool
fat_cluster_free_p (uint32_t cluster)
{
    return cluster == CLUST_FREE;
}


/* Return true if cluster is the last in the chain.  */
bool
fat_cluster_last_p (uint32_t cluster)
{
    return cluster >= CLUST_EOFS;
}



/**
 * Convert a cluster number to a sector number.
 * @param cluster Cluster number
 * @return Sector number
 */
uint32_t
fat_cluster_to_sector (fat_t *fat, uint32_t cluster)
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
fat_cluster_entry_get (fat_t *fat, uint32_t cluster)
{
    uint32_t sector, offset, cluster_new, mask;
    uint8_t *buffer;
    
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
    buffer = fat_io_cache_read (fat, sector);
    
    /* Get the data for desired FAT entry.  */
    offset = offset % fat->bytes_per_sector;
    if (fat->type == FAT_FAT32)
        cluster_new = le32_get (buffer + offset);
    else
        cluster_new = le16_get (buffer + offset);

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

   cluster_new = fat_cluster_entry_get (fat, cluster);

   if (fat_cluster_free_p (cluster_new))
   {
       TRACE_ERROR (FAT, "FAT:Entry %u free\n", (unsigned int) cluster);
       return CLUST_EOFE;
   }
   return cluster_new;
}


/* Set a FAT entry.  */
static void
fat_cluster_entry_set (fat_t *fat, uint32_t cluster, uint32_t cluster_new)
{
    uint32_t sector, offset;
    uint8_t *buffer;
    
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
    buffer = fat_io_cache_read (fat, sector);

    /* Set the data for desired FAT entry.  */
    offset = offset % fat->bytes_per_sector;

    if (fat->type == FAT_FAT32)
    {
        le32_set (buffer + offset, cluster_new);
    }
    else
    {
        le16_set (buffer + offset, cluster_new);        
    }

    fat_io_cache_write (fat, sector);
}


uint16_t 
fat_cluster_chain_length (fat_t *fat, uint32_t cluster)
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


uint32_t
fat_cluster_free_search (fat_t *fat, uint32_t start, uint32_t stop)
{
    uint32_t cluster;

    /* Linearly search through the FAT looking for a free cluster.  */
    for (cluster = start; cluster < stop; cluster++)
    {
        if (fat_cluster_free_p (fat_cluster_entry_get (fat, cluster)))
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

    cluster_start = fat_fsinfo_prev_free_cluster_get (fat) + 1;

    cluster = fat_cluster_free_search (fat, cluster_start, 
                                       fat->num_clusters);
    if (!cluster)
        cluster = fat_cluster_free_search (fat, CLUST_FIRST, 
                                           cluster_start);        

    /* Out of memory.  */
    if (!cluster)
        return 0;

    fat_fsinfo_free_clusters_update (fat, -1);
    fat_fsinfo_prev_free_cluster_set (fat, cluster);
    return cluster;
}


static uint32_t
fat_cluster_chain_append (fat_t *fat, uint32_t cluster_start,
                          uint32_t cluster_new)
{
    if (!fat_cluster_last_p (fat_cluster_entry_get (fat, cluster_start)))
        TRACE_ERROR (FAT, "FAT:Bad chain\n");

    fat_cluster_entry_set (fat, cluster_start, cluster_new);

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
        fat_cluster_entry_set (fat, cluster_last, 0x00000000);
        count++;
    }

    fat_fsinfo_free_clusters_update (fat, count);
    fat_fsinfo_write (fat);
} 



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
    fat_cluster_entry_set (fat, cluster, CLUST_EOFE);    

    /* Append to cluster chain.  */
    if (cluster_start)
        fat_cluster_chain_append (fat, cluster_start, cluster);

    return cluster;
}


uint32_t
fat_cluster_chain_extend (fat_t *fat, uint32_t cluster_start, 
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
fat_cluster_stats (fat_t *fat, fat_cluster_stats_t *stats)
{
    uint32_t cluster;
    uint32_t free_clusters;
    bool found;
    
    found = 0;
    free_clusters = 0;

    /* Iterate over all clusters counting allocated clusters.  */
    for (cluster = CLUST_FIRST; cluster < fat->num_clusters; cluster++)
    {
        if (fat_cluster_free_p (fat_cluster_entry_get (fat, cluster)))
        {
            free_clusters++;
            if (!found)
            {
                /* This is not quite correct; it should be the last
                   allocated cluster.  */
                stats->prev_free_cluster = cluster;
                found = 1;
            }
        }
    }

    stats->total = fat->num_clusters;
    stats->free = free_clusters;
    stats->alloc = stats->total - stats->free;
}


void
fat_cluster_stats_dump (fat_t *fat)
{
    fat_cluster_stats_t stats;

    fat_cluster_stats (fat, &stats);
    
    TRACE_ERROR (FAT, "Free  %" PRIu32 "\n", stats.free);
    TRACE_ERROR (FAT, "Alloc %" PRIu32 "\n", stats.alloc);
    TRACE_ERROR (FAT, "Total %" PRIu32 "\n", stats.total);
}


void
fat_cluster_chain_dump (fat_t *fat, uint32_t cluster)
{
    while (1)
    {
        TRACE_INFO (FAT, "%lu ", cluster);        
        if (!cluster)
            break;

        cluster = fat_cluster_next (fat, cluster);
        if (fat_cluster_last_p (cluster))
            break;
    }
    TRACE_INFO (FAT, "\n");
}

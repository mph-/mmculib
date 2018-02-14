/** @file   fat_cluster.h
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem FAT cluster manipulation routines.
*/


#ifndef FAT_CLUSTER_H
#define FAT_CLUSTER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "fat.h"


/* Cluster statistics structure.  */
typedef struct fat_cluster_stats_struct
{
    uint32_t total;
    uint32_t free;
    uint32_t alloc;
    uint32_t prev_free_cluster;
} fat_cluster_stats_t;


uint32_t fat_cluster_to_sector (fat_t *fat, uint32_t cluster);


uint32_t fat_cluster_chain_extend (fat_t *fat, uint32_t cluster_start, 
                                   uint32_t num_clusters);


uint16_t fat_cluster_chain_length (fat_t *fat, uint32_t cluster);


void fat_cluster_chain_free (fat_t *fat, uint32_t cluster_start);


uint32_t fat_cluster_next (fat_t *fat, uint32_t cluster);


/** Return true if cluster is the last in the chain.  */
bool fat_cluster_last_p (uint32_t cluster);


/** Return true if cluster unallocated.  */
bool fat_cluster_free_p (uint32_t cluster);


void fat_cluster_stats (fat_t *fat, fat_cluster_stats_t *stats);


void fat_cluster_chain_dump (fat_t *fat, uint32_t cluster);


#ifdef __cplusplus
}
#endif    
#endif


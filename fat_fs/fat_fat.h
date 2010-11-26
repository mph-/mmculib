/** @file   fat_fat.h
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem FAT manipulation routines.
*/


#ifndef FAT_FAT_H
#define FAT_FAT_H

#include "fat.h"

uint32_t fat_cluster_to_sector (fat_t *fat, uint32_t cluster);


uint32_t fat_cluster_chain_extend (fat_t *fat, uint32_t cluster_start, 
                           uint32_t num_clusters);


uint16_t fat_cluster_chain_length (fat_t *fat, uint32_t cluster);


void fat_cluster_chain_free (fat_t *fat, uint32_t cluster_start);


uint32_t fat_cluster_next (fat_t *fat, uint32_t cluster);


/* Return true if cluster is the last in the chain.  */
bool fat_cluster_last_p (uint32_t cluster);





#endif

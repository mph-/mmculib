/** @file   fat_stats.h
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem statistics.
*/


#ifndef FAT_STATS_H
#define FAT_STATS_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "fat.h"


/* Stats statistics structure.  */
typedef struct fat_stats_struct
{
    uint32_t total;
    uint32_t free;
    uint32_t alloc;
    uint32_t prev_free_stats;
} fat_stats_t;


void fat_stats (fat_t *fat, fat_stats_t *stats);


#ifdef __cplusplus
}
#endif    
#endif


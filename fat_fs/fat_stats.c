#include "fat_stats.h"

void fat_stats (fat_t *fat, fat_stats_t *stats)
{
    fat_cluster_stats_t cluster_stats;
    uint32_t cluster_kB;

    cluster_kB = fat->bytes_per_sector * fat->sectors_per_cluster / 1024;

    fat_cluster_stats (fat, &cluster_stats);
    stats->free = cluster_stats.free * cluster_kB;
    stats->alloc = cluster_stats.alloc * cluster_kB;
    stats->total = cluster_stats.total * cluster_kB;
}



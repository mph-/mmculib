/** @file   fat_de.c
    @author Michael Hayes
    @date   23 November 2010
    @brief  FAT filesystem directory entry operations.
*/

#ifndef FAT_DE_H
#define FAT_DE_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "fat.h"
#include "fat_de.h"


/** The maximum length of a file or directory name.  */
#define FAT_NAME_LEN 256


/** Address of a directory entry.  */
struct fat_dir_struct
{
    uint32_t sector;
    uint16_t offset;
};


/** File find structure.  */
struct fat_ff_struct
{
    uint32_t parent_dir_cluster;
    uint32_t cluster;
    uint32_t size;
    struct fat_dir_struct dir;
    uint8_t  short_name[12];     //!< DOS version of the (short) name
    char     name[FAT_NAME_LEN]; //!< Name entry (long or short) 
    bool     isdir;              //!< Set to indicate a directory
};              


typedef struct fat_ff_struct fat_ff_t;

typedef struct fat_dir_struct fat_dir_t;


bool
fat_de_find (fat_t *fat, uint32_t dir_cluster, 
             const char *name, fat_ff_t *ff);

bool
fat_de_add (fat_t *fs, fat_dir_t *dir, 
            const char *filename, uint32_t cluster_dir);


void
fat_de_cluster_set (fat_t *fat, fat_dir_t *dir, uint32_t cluster);


void
fat_de_size_set (fat_t *fs, fat_dir_t *dir, uint32_t size);


void
fat_de_dir_dump (fat_t *fat, uint32_t dir_cluster);


void
fat_de_slot_delete (fat_t *fat, fat_dir_t *dir, uint32_t cluster);


#ifdef __cplusplus
}
#endif    
#endif


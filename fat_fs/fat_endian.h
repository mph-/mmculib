/** @file   fat_endian.h
    @author Michael Hayes
    @date   7 January 2009
    @brief  FAT endian conversion routines.
*/
#ifndef FAT_ENDIAN_H
#define FAT_ENDIAN_H

#ifdef __cplusplus
extern "C" {
#endif
    

static inline uint16_t le16_to_cpu (uint16_t val)
{
    return val;
}


static inline uint32_t le32_to_cpu (uint32_t val)
{
    return val;
}


static inline uint16_t cpu_to_le16 (uint16_t val)
{
    return val;
}


static inline uint32_t cpu_to_le32 (uint32_t val)
{
    return val;
}


static inline uint16_t le16_get (void *ptr)
{
    return le16_to_cpu (*(uint16_t *)ptr);
}


static inline uint32_t le32_get (void *ptr)
{
    return le32_to_cpu (*(uint32_t *)ptr);
}


static inline void le16_set (void *ptr, uint16_t val)
{
    *(uint16_t *)ptr = cpu_to_le16 (val);
}


static inline void le32_set (void *ptr, uint32_t val)
{
    *(uint32_t *)ptr = cpu_to_le32 (val);
}



#ifdef __cplusplus
}
#endif    
#endif


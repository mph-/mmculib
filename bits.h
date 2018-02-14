/** @file   bits.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#ifndef BITS_H
#define BITS_H

#ifdef __cplusplus
extern "C" {
#endif
    

#define WORD_HIGHBIT (sizeof(unsigned) * 8 - 1)
#define BITS_MASK(first, last) (((unsigned) -1 >> (WORD_HIGHBIT - (last))) & ~((1U << (first)) - 1))

#define BITS_CLR(reg, first, last) ((reg) &= BITS_MASK ((first), (last)))

#define BITS_SET(reg, first, last) ((reg) |= BITS_MASK ((first), (last)))

#define BITS_EXTRACT(reg, first, last) \
    (((reg) & BITS_MASK ((first), (last))) >> (first))

#define BITS_INSERT(reg, val, first, last) \
    (reg) = ((reg) & ~BITS_MASK ((first), (last)))  \
        | (((val) & BITS_MASK (0, (last) - (first))) << (first))

#define BITS(val, first, last) \
    (((val) & BITS_MASK (0, (last) - (first))) << (first))


#ifdef __cplusplus
}
#endif    
#endif


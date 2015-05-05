/** @file   bits.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#ifndef BITS_H
#define BITS_H


#define BITS_MASK(first, last) ((1 << ((last) + 1)) - (1 << (first)))

#define BITS_CLR(reg, first, last) ((reg) &= BITS_MASK ((first), (last)))

#define BITS_SET(reg, first, last) ((reg) |= BITS_MASK ((first), (last)))

#define BITS_EXTRACT(reg, first, last) \
    (((reg) & BITS_MASK ((first), (last))) >> (first))

#define BITS_INSERT(reg, val, first, last) \
    (reg) = ((reg) & ~BITS_MASK ((first), (last)))  \
        | (((val) & BITS_MASK (0, (last) - (first))) << (first))

#define BITS(val, first, last) \
    (((val) & BITS_MASK (0, (last) - (first))) << (first))

#endif

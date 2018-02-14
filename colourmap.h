/** @file   colourmap.h
    @author M. P. Hayes, UCECE
    @date   3 July 2007
    @brief 
*/

#ifndef COLOURMAP_H
#define COLOURMAP_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

/* The only constraints on the colourmap is that the first entry is
   for black and the last entry is white.  We could pack the colourmap
   entries to save memory by changing the definition of
   COLOURMAP_ENTRY.  With 5 bits per colour a colourmap entry could be
   packed into 16 bits rather than the current 24 bits.  */


/* COLOURMAP_SCALE needs to be defined by the user.  */


#ifndef COLOURMAP_R_WEIGHT
#define COLOURMAP_R_WEIGHT 1.0
#endif

#ifndef COLOURMAP_G_WEIGHT
#define COLOURMAP_G_WEIGHT 1.0
#endif

#ifndef COLOURMAP_B_WEIGHT
#define COLOURMAP_B_WEIGHT 1.0
#endif


#define COLOURMAP_R(X) ((X) * COLOURMAP_R_WEIGHT * COLOURMAP_SCALE + 0.5)
#define COLOURMAP_G(X) ((X) * COLOURMAP_G_WEIGHT * COLOURMAP_SCALE + 0.5)
#define COLOURMAP_B(X) ((X) * COLOURMAP_B_WEIGHT * COLOURMAP_SCALE + 0.5)

#define COLOURMAP_ENTRY(R, G, B) \
{COLOURMAP_R(R), COLOURMAP_G(G), COLOURMAP_B(B)}, 


typedef uint8_t colourmap_elt_t;

typedef colourmap_elt_t colourmap_t[3];



#ifdef __cplusplus
}
#endif    
#endif


/** @file   fontdef.h
    @author M. P. Hayes, UCECE
    @date   1 March 2007
    @brief  Font definition.
*/

#ifndef FONTDEF_H
#define FONTDEF_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"


typedef const uint8_t font_data_t;

/** Font structure.  */
typedef const struct
{
    /* Flags for future options.  */
    uint8_t flags;
    /* Width of font char.  */
    uint8_t width;
    /* Height of font char.  */
    uint8_t height;
    /* Index of first entry in font.  */
    uint8_t offset;
    /* Number of font entries in table.  */
    uint8_t size;
    /* Font data entries.  */
    font_data_t data[];
} font_t;



#ifdef __cplusplus
}
#endif    
#endif


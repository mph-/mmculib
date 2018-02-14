/** @file   stext.h
    @author M. P. Hayes, UCECE
    @date   2 April 2007
    @brief  Sequenced text
*/
#ifndef STEXT_H
#define STEXT_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "font.h"
#include "seq.h"


typedef enum {STEXT_MODE_NORMAL, 
              STEXT_MODE_CYCLE, 
              STEXT_MODE_NUM} stext_mode_t;

typedef struct
{
    seq_t seq;
    font_t *font;
    void (*callback) (void *data, uint8_t pixel, bool val);
    void *callback_data;
    seq_obj_t seq_info;
} stext_obj_t;


typedef stext_obj_t *stext_t;

extern stext_t
stext_init (stext_obj_t *dev, 
            font_t *font,
            void (*callback) (void *data, uint8_t pixel, bool val),
            void *callback_data);


/* Set the string to display.  NB, the string pointed to by STR is not
   copied and thus must be static.  */
static inline void
stext_set (stext_t stext, const char *str)
{
    seq_set (stext->seq, str);
}


static inline const char *
stext_get (stext_t stext)
{
    return seq_get (stext->seq);
}


/* Returns non-zero at end of sequence.  */
static inline int8_t
stext_update (stext_t stext)
{
    return seq_update (stext->seq);
}

#ifdef __cplusplus
}
#endif    
#endif


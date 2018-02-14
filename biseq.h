/** @file   biseq.h
    @author M. P. Hayes, UCECE
    @date   1 April 2007
    @brief 
*/
#ifndef BISEQ_H
#define BISEQ_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

typedef enum {BISEQ_MODE_NORMAL, 
              BISEQ_MODE_CYCLE, 
              BISEQ_MODE_NUM} biseq_mode_t;

typedef struct
{
    char *str;
    uint8_t step;
    int8_t dir;
    biseq_mode_t mode;
    int8_t (*callback) (void *data, char *str);
    void *callback_data;
} biseq_obj_t;


typedef struct biseq_struct *biseq_t;


extern biseq_t biseq_init (biseq_obj_t *dev, 
                       int8_t (*callback) (void *data, char *str),
                       void *callback_data);

extern void biseq_set (biseq_t biseq, char *str);

extern char *biseq_get (biseq_t biseq);

extern void biseq_mode_set (biseq_t biseq, biseq_mode_t mode);

extern biseq_mode_t biseq_mode_get (biseq_t biseq);

/* Returns non-zero at end of bisequence.  */
extern int8_t biseq_update (biseq_t biseq);


#ifdef __cplusplus
}
#endif    
#endif


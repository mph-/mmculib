/** @file   muxleds.h
    @author M. P. Hayes, UCECE
    @date   08 June 2002
    @brief 
*/
#ifndef MUXLEDS_H
#define MUXLEDS_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "port.h"


#ifndef MUXLEDS_ROWS_NUM
#define MUXLEDS_ROWS_NUM 8
#endif

#ifndef MUXLEDS_COLS_NUM
#define MUXLEDS_COLS_NUM 8
#endif


//#if MUXLEDS_ROWS_NUM > 8
//#error Too many LEDs in a row
//#endif


#define MUXLED_ROW_CFG(PORT, PORTBIT) {(PORT), BIT (PORTBIT)}
#define MUXLED_COL_CFG(PORT, PORTBIT) {(PORT), BIT (PORTBIT)}

typedef struct
{
    port_t port;
    uint8_t bitmask;
} muxleds_cfg_t;


/* These structures are defined here so the compiler can allocate enough
   memory for them.  However, its fields should be treated as
   private.  */
typedef struct
{
    port_t port;
    uint8_t bitmask;
} muxleds_row_t;


typedef struct
{
    port_t port;
    uint8_t bitmask;
    uint8_t row_state;
} muxleds_col_t;


typedef struct
{
    muxleds_row_t rows[MUXLEDS_ROWS_NUM];
    muxleds_col_t cols[MUXLEDS_COLS_NUM];
    uint8_t col;
    uint8_t row_on;
    uint8_t rows_num;
    uint8_t cols_num;
} muxleds_obj_t;


#ifndef MUXLEDS_TRANSPARENT
/* By default use opaque pointers.  */
typedef struct muxleds_struct *muxleds_t;
#else
typedef muxleds_obj_t *muxleds_t;
#endif


extern void
muxleds_set (muxleds_t muxleds, uint8_t bit, uint8_t val);

extern void
muxleds_toggle (muxleds_t muxleds, uint8_t bit);

extern void
muxleds_update (muxleds_t muxleds);

/* INFO is a pointer into RAM that stores the state of the MUXLEDS.
   CFG is a pointer into ROM to define the port the MUXLEDS is connected to.
   The returned handle is passed to the other muxleds_xxx routines to denote
   the MUXLEDS to operate on.  */
extern muxleds_t
muxleds_init (muxleds_obj_t *dev, const muxleds_cfg_t *row_cfg,
             uint8_t row_size,
             const muxleds_cfg_t *col_cfg,
             uint8_t col_size);

#ifdef __cplusplus
}
#endif    
#endif


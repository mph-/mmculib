/** @file   lmatrix.h
    @author M. P. Hayes, UCECE
    @date   28 March 2007
    @brief  Drive a multiplexed LED matrix.
    @note   This only supports a single instance.
*/
#ifndef LMATRIX_H
#define LMATRIX_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "port.h"


enum {LMATRIX_PIXELS = LMATRIX_ROWS * LMATRIX_COLS};


typedef struct
{
    port_t port;
    port_mask_t bitmask;
} lmatrix_port_t;

typedef uint8_t lmatrix_row_state_t;


/** This structure is defined here so the compiler can allocate enough
   memory for it.  However, its fields should be treated as
   private.  */
typedef struct
{
    lmatrix_port_t *col_port;
    uint8_t col;
    lmatrix_row_state_t state[LMATRIX_COLS];
} lmatrix_private_t;


typedef lmatrix_private_t lmatrix_obj_t;
typedef lmatrix_obj_t *lmatrix_t;


/** Set a pixel in the LED matrix to state VAL at ROW and COL.  */
extern void
lmatrix_set (lmatrix_t lmatrix, uint8_t row, uint8_t col, bool val);


extern void
lmatrix_write (lmatrix_t lmatrix, uint8_t *screen, uint8_t *map);


/** Scan the LED matrix.  This needs to be called at a rate of at least
   20 Hz * LMATRIX_COLS to avoid noticeable flicker.  */
extern void
lmatrix_update (lmatrix_t lmatrix);


/** INFO is a pointer into RAM that stores the state of the LMATRIX.
   The returned handle is passed to the other lmatrix_xxx routines to
   denote the LMATRIX to operate on.  Currently only one lmatrix
   instance is supported with parameters specified by target.h.  */
extern lmatrix_t
lmatrix_init (lmatrix_obj_t *info);

#ifdef __cplusplus
}
#endif    
#endif


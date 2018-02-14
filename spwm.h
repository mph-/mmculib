/** @file   spwm.h
    @author M. P. Hayes, UCECE
    @date   13 March 2005
    @brief 
*/
#ifndef SPWM_H
#define SPWM_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "port.h"

/* This structure is defined here so the compiler can allocate enough
   memory for it.  However, its fields should be treated as
   private.  */
typedef struct
{
    uint8_t period;
    uint8_t duty;
    uint8_t count;
} spwm_obj_t;


typedef struct spwm_struct *spwm_t;


extern void spwm_period_set (spwm_t spwm, uint16_t period);

extern void spwm_duty_set (spwm_t spwm, uint16_t duty);


extern bool
spwm_update (spwm_t);


/* INFO is a pointer into RAM that stores the state of the FLASH.
   CFG is a pointer into ROM to define the port the FLASH is connected to.
   The returned handle is passed to the other spwm_xxx routines to denote
   the FLASH to operate on.  */
extern spwm_t
spwm_init (spwm_obj_t *info);

#ifdef __cplusplus
}
#endif    
#endif


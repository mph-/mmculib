/** @file   mpwm.h
    @author M. P. Hayes, UCECE
    @date   1 April 2007
    @brief 
*/
#ifndef MPWM_H
#define MPWM_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"


/* These structures are defined here so the compiler can allocate
   enough memory for them.  However, their fields should be treated as
   private.  */
typedef struct
{
    uint8_t duty;
} mpwm_channel_t;


typedef struct
{
    uint8_t period;
    uint8_t count;
    mpwm_channel_t *channels;
    uint8_t num_channels;
} mpwm_obj_t;


typedef struct mpwm_struct *mpwm_t;


extern void mpwm_period_set (mpwm_t mpwm, uint16_t period);

extern void mpwm_duty_set (mpwm_t mpwm, uint8_t channel, uint16_t duty);


extern bool
mpwm_update (mpwm_t);


/* INFO is a pointer into RAM that stores the state of the FLASH.
   CFG is a pointer into ROM to define the port the FLASH is connected to.
   The returned handle is passed to the other mpwm_xxx routines to denote
   the FLASH to operate on.  */
extern mpwm_t
mpwm_init (mpwm_obj_t *info, mpwm_channel_t *channels, uint8_t num_channels);

#ifdef __cplusplus
}
#endif    
#endif


/** @file   pulser.h
    @author M. P. Hayes, UCECE
    @date   31 December 2007
    @brief  HV pulser for Treetap6.
*/
#ifndef PULSER_H
#define PULSER_H

#include "config.h"



/** Initialise pulser.  */
extern void 
pulser_init (void);


extern void
pulser_fire (uint8_t channel);


extern void
pulser_on (uint8_t channel);


extern void
pulser_off (uint8_t channel);


extern void 
pulser_hv_enable (void);


extern void 
pulser_hv_disable (void);


extern bool
pulser_hv1_ready_p (void);


extern bool
pulser_hv2_ready_p (void);


extern void 
pulser_shutdown (void);


#endif /* PULSER_H  */

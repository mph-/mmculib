/** @file   flasher_tweak.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#ifndef FLASHER_TWEAK_H
#define FLASHER_TWEAK_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "flasher.h"

extern void
flasher_tweak_mod_duty (flasher_pattern_t *pattern, uint8_t mod_duty);

extern void
flasher_tweak_mod_freq (flasher_pattern_t *pattern, uint16_t poll_freq, 
                        uint8_t mod_freq);

#ifdef __cplusplus
}
#endif    
#endif


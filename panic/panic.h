/** @file   panic.h
    @author M. P. Hayes, UCECE
    @date   28 Jan 2021
    @brief  Panic support.
*/
#ifndef PANIC_H
#define PANIC_H

#include "pio.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialise panic:
    @param panic_rate rate in Hz.  */
extern void panic (pio_t error_led_pio, unsigned int error_code);


#ifdef __cplusplus
}
#endif
#endif /* PANIC_H  */

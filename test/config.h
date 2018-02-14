#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
    

/* Data typedefs.  */
/* Data typedefs.  */
#include <stdint.h>

typedef uint8_t bool;

#include "target.h"

#ifdef AVR
#include <util/delay.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#define HOSTED 0
#else
// #include "avrsim.h"
#define HOSTED 1
#endif

#ifndef _BV
#define _BV(X) (1 << (X))
#endif

#define BIT(X) _BV(X)

/* Macros to set, check and clear bits.  */
#define BSET(port, pin)  (port |= (BIT (pin)))
#define BCLR(port, pin)  (port &= ~(BIT (pin)))
#define BTST(port, pin)  (port & (BIT (pin)))

#define ARRAY_SIZE(array) (sizeof(array) / sizeof (array[0]))



#ifdef __cplusplus
}
#endif    
#endif



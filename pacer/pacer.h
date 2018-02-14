/** @file   pacer.h
    @author M. P. Hayes, UCECE
    @date   23 August 2010
    @brief  Paced loop support.

    @defgroup pacer Paced loop module
       
    This module provides support for paced loops by abstracting a
    hardware timer.

    Here's a simple example for a paced loop that operates at 1 kHz.

       @code
       #include "pacer.h"

       void main (void)
       {
           pacer_init (1000);

           while (1)
           {
               pacer_wait ();

               // Do something.
           }
        }
       @endcode
*/
#ifndef PACER_H
#define PACER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

/** Define size of pacer rates.  */
typedef uint16_t pacer_rate_t;


/** Wait for the next pacer tick.  */
extern void pacer_wait (void);


/** Initialise pacer:
    @param pacer_rate rate in Hz.  */
extern void pacer_init (pacer_rate_t pacer_rate);


#ifdef __cplusplus
}
#endif    
#endif /* PACER_H  */


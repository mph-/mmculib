/** @file   ir_rc5_rx.h
    @author M. P. Hayes, UCECE
    @date   21 May 2013
    @brief  Infrared serial receiver driver for Phillips RC5 protocol.
*/

#ifndef IR_RC5_RX_H
#define IR_RC5_RX_H

#include "config.h"

/** Status return codes.  */
typedef enum ir_rc5_rx_ret 
{
    /** No data to read.  */
    IR_RC5_RX_NONE = 0,
    /** Timeout waiting for transition.  */
    IR_RC5_RX_TIMEOUT = -1
} ir_rc5_rx_ret_t;


/** Receive RC5 data packet over IR serial link.  
    @return 14-bits of data or error status code
    @note No error checking is performed.  If there is no activity on the
    IR serial link, this function returns immediately.  Otherwise, this
    function blocks until the entire frame is received.  This must be called
    frequently to ensure that a start bit is seen.  */
int16_t ir_rc5_rx_read (void);


/** Initialise IR serial receiver driver.  */
void ir_rc5_rx_init (void);

#endif

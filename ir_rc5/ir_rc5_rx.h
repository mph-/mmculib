/** @file   ir_rc5.c
    @author M. P. Hayes, UCECE
    @date   21 May 2013
    @brief  Infrared serial driver for Phillips RC5 protocol.
*/

#ifndef IR_RC5_RX_H
#define IR_RC5_RX_H

#include "config.h"

/** Status return codes.  */
typedef enum ir_rc5_rx_ret 
{
    /** A valid frame has been received.  */
    IR_RC5_RX_OK = 1,
    /** No data to read.  */
    IR_RC5_RX_NONE = 0,
} ir_rc5_rx_ret_t;


/** Receive RC5 data packet over IR serial link.  
    @param psystem pointer to byte to store received system data
    @param pcode pointer to byte to store received code
    @param ptoggle pointer to byte to store toggle status
    @return status code
    @note No error checking is performed.  If there is no activity on the
    IR serial link, this function returns immediately.  Otherwise, this
    function blocks until the entire frame is received.  This must be called
    frequently to ensure that a start bit is seen.  */
ir_rc5_rx_ret_t ir_rc5_rx_read (uint8_t *psystem, uint8_t *pcode, uint8_t *ptoggle);


/** Initialise IR serial driver.  */
void ir_rc5_rx_init (void);

#endif

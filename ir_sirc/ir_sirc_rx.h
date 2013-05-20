/** @file   ir_sirc.c
    @author M. P. Hayes, UCECE
    @date   21 May 2013
    @brief  Infrared serial driver for Sony SIRC protocol.
*/

#ifndef IR_SIRC_RX_H
#define IR_SIRC_RX_H

#include "config.h"

/** Status return codes.  */
typedef enum ir_sirc_rx_ret 
{
    /** A valid frame has been received.  */
    IR_SIRC_RX_OK = 1,
    /** No data to read.  */
    IR_SIRC_RX_NONE = 0,
    /** Invalid start code detected.  */
    IR_SIRC_RX_START_ERR = -1,
    /** Invalid data code detected.  */
    IR_SIRC_RX_DATA_ERR = -2,
    /** Invalid break code detected.  */
    IR_SIRC_RX_BREAK_ERR = -3,
    /** Invalid stop code detected.  */
    IR_SIRC_RX_STOP_ERR = -4
} ir_sirc_rx_ret_t;


/** Receive 20 bits of data over IR serial link.  
    @param pcommand pointer to byte to store received command
    @param paddress pointer to word to store received address
    @return status code
    @note No error checking is performed.  If there is no activity on the
    IR serial link, this function returns immediately.  Otherwise, this
    function blocks until the entire frame is received.  */
ir_sirc_rx_ret_t ir_sirc_rx_read (uint8_t *pcommand, uint16_t *paddress);


/** Initialise IR serial driver.  */
void ir_sirc_rx_init (void);

#endif

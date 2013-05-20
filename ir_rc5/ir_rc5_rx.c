/** @file   ir_rc5_rx.c
    @author M. P. Hayes, UCECE
    @date   21 May 2013
    @brief  Infrared serial driver for Phillips RC5 protocol.

    @note   This has not been tested; it may not even be close to the 
    Phillips RC5 protocol!
*/


#include "ir_rc5_rx.h"
#include "pio.h"
#include "delay.h"


#ifndef IR_RC5_RX_ACTIVE_STATE
#define IR_RC5_RX_ACTIVE_STATE 0
#endif


/** Return output state of IR receiver.
    @return IR receiver state (1 = IR modulation detected).  */
static inline uint8_t ir_rc5_rx_get (void)
{
    /* The output of most IR receivers are normally high but go low
       when modulated IR light is detected.  */

    return pio_input_get (IR_RC5_RX_PIO) == IR_RC5_RX_ACTIVE_STATE;
}


uint8_t ir_rc5_rx_ready_p (void)
{
    return ir_rc5_rx_get ();
}


static inline void ir_rc5_rx_delay (uint16_t us)
{
    for (; us; us--)
    {
        DELAY_US (1);
    }
}


static uint16_t ir_rc5_rx_wait_state (uint8_t state)
{
    uint8_t us;
    
    /* TODO: Add timeout.  Should do some debouncing to reduce effects
       of interference.  */
    for (us = 0; ir_rc5_rx_get () != state; us++)
    {
        DELAY_US (1);
    }
    return us;
}


static uint16_t ir_rc5_rx_wait_transition (void)
{
    uint8_t initial;
    
    initial = ir_rc5_rx_get ();

    return ir_rc5_rx_wait_state (!initial);
}


static uint16_t ir_rc5_rx_wait_idle (void)
{
    return ir_rc5_rx_wait_state (0);
}


static uint8_t ir_rc5_rx_read_bits (uint8_t bits, uint16_t halfbit)
{
    uint8_t i;
    uint8_t data = 0;  
    
    for (i = 0; i < bits; i++) 
    {
        ir_rc5_rx_wait_transition ();

        ir_rc5_rx_delay (halfbit >> 1);

        if (ir_rc5_rx_get ())
            data |= BIT (i);
    }
    return i;
}


/** Receive RC5 data packet over IR serial link.  
    @param psystem pointer to byte to store received system data
    @param pcode pointer to byte to store received code
    @return status code
    @note No error checking is performed.  If there is no activity on the
    IR serial link, this function returns immediately.  Otherwise, this
    function blocks until the entire frame is received.  This must be called
    frequently to ensure that a start bit is seen.  */
ir_rc5_rx_ret_t ir_rc5_rx_read (uint8_t *psystem, uint8_t *pcode)
{
    uint8_t system;
    uint8_t code;
    uint8_t key;
    uint16_t halfbit;

    /* Here's the algorithm:
       - Wait for the input to go idle again (end of the second half
         of start bit #1), start counting
       - Count until the input goes low again (middle of start bit
         #2), this is the halfbit width
       - Wait 1.5 halfbit widths, which takes us somewhere in the
         middle of the first half of the first data bit
       - Wait for the edge in the middle of the bit
       - Wait a half-halfbit width
       - Sample.
       - Wait a halfbit width
       - Wait for an edge
       - Repeat 
    */

    if (!ir_rc5_rx_ready_p ())
        return IR_RC5_RX_NONE;

    /* Wait for end of second half of first start bit.  */
    ir_rc5_rx_wait_idle ();

    /* Determine period of first half of second start bit.  */
    halfbit = ir_rc5_rx_wait_transition ();

    /* Delay into the middle of the first half of the first data bit.  */
    ir_rc5_rx_delay (halfbit + (halfbit >> 1));

    /* Read key bit.  */
    key = ir_rc5_rx_read_bits (1, halfbit);

    /* Read system bits.  */
    system = ir_rc5_rx_read_bits (5, halfbit);

    /* Read code bits.  */
    code = ir_rc5_rx_read_bits (6, halfbit);

    *psystem = system;
    *pcode = code;

    return IR_RC5_RX_OK;
}


/** Initialise IR serial receiver driver.  */
void ir_rc5_rx_init (void)
{
    /* Configure IR receiver PIO as input.  */
    pio_config_set (IR_RC5_RX_PIO, PIO_INPUT);
}


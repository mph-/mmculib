/** @file   ir_rc5_rx.c
    @author M. P. Hayes, UCECE
    @date   21 May 2013
    @brief  Infrared serial receiver driver for Phillips RC5 protocol.
*/

/* The Phillips RC-5 protocol uses the following format of 14 bits:
   1 start bit
   1 field bit (this used to be a second start bit)
   1 toggle bit
   5 address bits
   6 command bits
 
   Manchester encoding is used for the bits.  The start bit is a `1' bits.
   `1' bits have the IR modulation on for the second half of the cycle.
   `0' bits have the IR modulation on for the first half of the cycle.

   Each bit period is 1778 microseconds (64 cycles of 36 kHz modulation)
   thus a frame is 24.9 ms.

   While a button is pressed, a frame is repeatedly sent.  If the button
   is repressed, the toggle bit is changed.

   This is a simple polled implementation that will block for the entire frame
   if it detects a start bit.
*/


#include "ir_rc5_rx.h"
#include "pio.h"
#include "delay.h"

#define IR_RC5_BIT_PERIOD_US 1778

#ifndef IR_RC5_RX_ACTIVE_STATE
/* Most IR demodulators have an active low output.  */
#define IR_RC5_RX_ACTIVE_STATE 0
#endif


/** Return output state of IR receiver.
    @return IR receiver state (1 = IR modulation detected).  */
static inline uint8_t ir_rc5_rx_get (void)
{
    return pio_input_get (IR_RC5_RX_PIO) == IR_RC5_RX_ACTIVE_STATE;
}


uint8_t ir_rc5_rx_ready_p (void)
{
    return ir_rc5_rx_get ();
}


static uint16_t ir_rc5_rx_wait_state (uint8_t state)
{
    uint16_t us;
    
    for (us = 0; us < IR_RC5_BIT_PERIOD_US; us++)
    {
        if (ir_rc5_rx_get () == state)
            return us;
        /* TODO: figure out how to determine the fudge term.  Ideally,
           we should poll a counter incremented by the CPU clock.  */
        DELAY_US (1 - 0.4);
    }
    return us;
}


static uint16_t ir_rc5_rx_wait_transition (void)
{
    uint8_t initial;
    
    initial = ir_rc5_rx_get ();

    return ir_rc5_rx_wait_state (!initial);
}


/** Receive RC5 data packet over IR serial link.  
    @return 14-bits of data or error status code
    @note No error checking is performed.  If there is no activity on the
    IR serial link, this function returns immediately.  Otherwise, this
    function blocks until the entire frame is received.  This must be called
    frequently to ensure that a start bit is seen.  */
int16_t ir_rc5_rx_read (void)
{
    int16_t data;
    int i;
    uint16_t us;

    /* Look to see if there is some IR modulation marking the second
       half of the first start bit.  It is possible that we may have
       missed the start bit.  In this case we are likely to
       time out.  */
    if (!ir_rc5_rx_ready_p ())
        return IR_RC5_RX_NONE;

    /* The old RC-5 format had two start bits; this made a bit-bashed
       software implementation easier.  The problem is that we have
       been called just before the falling edge of the start bit.
       So we have to special case these two bits.  */

    /* Search for the next falling transition.  */
    us = ir_rc5_rx_wait_state (0);
    if (us >= IR_RC5_BIT_PERIOD_US)
        return IR_RC5_RX_TIMEOUT;

    if (us > (IR_RC5_BIT_PERIOD_US >> 1))
    {
        /* The field bit is 0.  */
        data = 2;
    }
    else
    {
        /* The field bit is 1; so delay until middle of bit period.  */
        data = 3;
        us = ir_rc5_rx_wait_state (1);
        if (us >= IR_RC5_BIT_PERIOD_US)
            return IR_RC5_RX_TIMEOUT;
    }

    for (i = 2; i < 14; i++)
    {
        data <<= 1;
        
        DELAY_US (0.5 * IR_RC5_BIT_PERIOD_US + 100);

        us = ir_rc5_rx_wait_transition ();
        if (us >= IR_RC5_BIT_PERIOD_US)
            return IR_RC5_RX_TIMEOUT;

        if (ir_rc5_rx_get ())
            data |= 1;
    }

    return data;
}


/** Initialise IR serial receiver driver.  */
void ir_rc5_rx_init (void)
{
    /* Ensure PIO clock activated for reading.  */
    pio_init (IR_RC5_RX_PIO);

    /* Configure IR receiver PIO as input.  */
    pio_config_set (IR_RC5_RX_PIO, PIO_INPUT);
}


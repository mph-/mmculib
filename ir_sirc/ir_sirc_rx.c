/** @file   ir_sirc.c
    @author M. P. Hayes, UCECE
    @date   21 May 2013
    @brief  Infrared serial driver for Sony SIRC protocol.

    @note   This has not been tested; it may not even be close to the 
    Sony SIRC protocol!
*/

/*
  This should be compiled with optimisation enabled otherwise some of
  the timing may be dodgy.

  The Sony SIRC protocol uses pulse width modulation.  A message
  consists of a start code, 7 data bits (transmitted LSB first), 13
  address bits (LSB first), and a stop code.  Each code is comprised
  of 0.6 ms intervals called a dit:

      Start bit: four dits on, one dit off

      One bit:   two dits on, one dit off

      Zero bit:  one dit on, one dit off

      Stop bit:  one dit off

    Note, the duration of a message depends on the number of non-zero
    bits in the data byte.  
*/


#include "ir_sirc.h"
#include "delay.h"

#ifndef IR_SIRC_DIT_PERIOD
#define IR_SIRC_DIT_PERIOD 0.6e-3
#endif


#ifndef IR_SIRC_ACTIVE_STATE
#define IR_SIRC_ACTIVE_STATE 0
#endif


#define IR_SIRC_DELAY_US 10
#define IR_SIRC_DIT_COUNT (1e6 * IR_SIRC_DIT_PERIOD / IR_SIRC_DELAY_US)
#define IR_SIRC_START_COUNT_MAX ((int)(4.5 * IR_SIRC_DIT_COUNT))
#define IR_SIRC_ONE_COUNT_MAX ((int)(2.5 * IR_SIRC_DIT_COUNT))
#define IR_SIRC_ZERO_COUNT_MAX ((int)(1.25 * IR_SIRC_DIT_COUNT))
#define IR_SIRC_BREAK_COUNT_MAX ((int)(1.5 * IR_SIRC_DIT_COUNT))


/** Return output state of IR receiver.
    @return IR receiver state (1 = IR modulation detected).  */
static inline uint8_t ir_sirc_rx_get (void)
{
    /* The output of most IR receivers are normally high but go low
       when modulated IR light is detected.  */

    return pio_input_get (IR_SIRC_RX_PIO) == IR_SIRC_ACTIVE_STATE;
}


/** Receive 20 bits of data over IR serial link.  
    @param pcommand pointer to byte to store received command
    @param paddress pointer to word to store received address
    @return status code
    @note No error checking is performed.  If there is no activity on the
    IR serial link, this function returns immediately.  Otherwise, this
    function blocks until the entire frame is received.  */
ir_sirc_ret_t ir_sirc_rx_read (uint8_t *pcommand, uint16_t *paddress)
{
    int i;
    int count;
    uint32_t data;
    bool data_err;

    /* Check for start code; if not present return.  */
    if (!ir_rx_get ())
        return IR_SIRC_NONE;

    /* Wait for end of start code or timeout.  */
    for (count = 0; ir_rx_get (); count++)
    {
        if (count >= IR_SIRC_START_COUNT_MAX)
            return IR_SIRC_START_ERR;

        DELAY_US (IR_SIRC_DELAY_US);
    }

    /* We may have received a rogue short pulse but we may have been
       called close to the falling transition.  */

    data_err = 0;
    data = 0;

    /* Seven command bits followed by 13 address bits; LSB first.  */
    for (i = 0; i < 20; i++)
    {
        data >>= 1;

        /* Wait for IR modulation to start or timeout (indicating
           detection of a false start code).  */
        for (count = 0; !ir_rx_get (); count++)
        {
            if (count >= IR_SIRC_BREAK_COUNT_MAX)
                return IR_SIRC_BREAK_ERR;

            DELAY_US (IR_SIRC_DELAY_US);
        }

        /* If there is another transmission in this slot we will get
           too small a value for count.  This is likely to trigger
           a bit error.  */
        for (count = 0; count < IR_SIRC_ONE_COUNT_MAX
                 && ir_rx_get (); count++)
        {
            DELAY_US (IR_SIRC_DELAY_US);
        }

        if (count >= IR_SIRC_ONE_COUNT_MAX)
            data_err = 1;
            
        if (count >= IR_SIRC_ZERO_COUNT_MAX)
            data |= 0x80;
    }

    /* Perhaps should check for stop code.  If not found, there is
       likely to have been interference from another transmitter.  */

    *pcommand = data & 0x7f;
    *paddress = data >> 7;
    return data_err ? IR_SIRC_DATA_ERR : IR_SIRC_OK;
}


/** Initialise IR serial receiver driver.  */
void ir_sirc_rx_init (void)
{
    /* Configure receiver PIO as input.  */
    pio_config_set (IR_SIRC_RX_PIO, PIO_INPUT);
}

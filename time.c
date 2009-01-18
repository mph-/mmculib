/** @file   time.c
    @author Ian Downes, UCECE
    @date   12 January 2005
    @brief  Time routines.
*/

/* MPH, this needs to be generalised for different microprocessors.  */

#include "time.h"
#include "delay.h"

static volatile uint16_t ms_ticks = 0;
//static volatile uint8_t compare_flag = 0;
//static volatile uint8_t data_flag = 0;

void
time_init (void)
{
    /* TCCR1A - setup for clear at top count mode
       COM1A1  0   normal port operation
       COM1A0  0   "
       COM1B1  0   "
       COM1B0  0   "
       FOC1A   0   no force output
       FOC1B   0   "
       WGM11   0   mode 4 (CTC)
       WGM10   0   "

    */
    TCCR1A = 0x00;

    /* TCCR1B - setup just for clk/1 input
       ICNC1   0   noise canceller
       ICES1   1   input capture edge rising
       RESERVED
       WGM13   0   mode 4 (CTC)
       WGM12   1   "
       CS12    0   prescaler
       CS11    0   "
       CS10    1   "
    */
    TCCR1B &= 0x20;
    TCCR1B |= (1 << ICES1) | (1 << WGM12) | (1 << CS10);

    /* TIMSK - setup just for int at clear
       OCIE2   -
       TOIE2   -
       TICIE1  1   input capture
       OCIE1A  1   output compare A
       OCIE1B  0   output compare B
       TOIE1   0   timer overflow
       not used
       TO1E0   -
    */
    TIMSK &= 0xC3;
    TIMSK |= (1 << TICIE1) | (1 << OCIE1A);

    /* OCR1A - setup count to clear at
       Compare register to set the top count
    */
    OCR1A = TOPCNT;

    /* Set up sleep mode.  */
    set_sleep_mode (SLEEP_MODE_IDLE);

    irq_global_enable ();
}


/* Delay function to handle delays that do not have predefined length.
 *
 * The delay is not accurate because the function and looping
 * overhead is not allowed for, so the real delay will always be
 * only slightly longer than the arg.
 *
 * The code is smaller than the old version and does not disable 
 * interrupts, which may cause have caused a 4ms rollover to be missed.
 */
void
time_delay_us (uint16_t us)
{
    uint16_t delay = us;

    while (delay >= 100)
    {
        DELAY_US (100);
        delay -= 100;
    }

    while (delay >= 10)
    {
        DELAY_US (10);
        delay -= 10;
    }

    while (delay >= 1)
    {
        DELAY_US (1);
        delay -= 1;
    }
}


/* This old version of the delay has a bug that occurs randomly where
 * the delay is attenuated to 7.68us.  Attempting to implement a simpler
 * delay using loops and delay_us
 */
/*
delay_ret_t
time_delay_us (uint16_t delay_us)
{
    uint16_t current_us_ticks;
    uint16_t delay;
    uint16_t timeout;

    irq_global_disable ();
    current_us_ticks = TCNT1;
    
    // Only support delays in given range.
    if (delay_us > DELAY_MAX)
        return ERRD;
    if (delay_us < DELAY_MIN)
        delay_us = DELAY_MIN;
    
    // Add correction for routine execution time.
    delay = (delay_us - 4) << 4;
    
    // Work out time for compare, account for wrap.
    timeout = delay + current_us_ticks;
    if ((timeout >= TOPCNT) || (timeout < current_us_ticks))
        timeout = timeout + (65536 - TOPCNT);
    if (timeout < 70) 
        timeout = 70; // What? Why?
    OCR1B = timeout;
    
    // Enable compare match interrupt.
    TIMSK = TIMSK | (1 << OCIE1B);
    // Enable capture interrupt.
    TIMSK = TIMSK | (1 << TICIE1);
    
    compare_flag = 0;
    data_flag = 0;

    irq_global_enable ();
    while ((compare_flag == 0) && (data_flag == 0))
    {
        sleep_mode ();
    }

    // Disable compare match interrupt.
    TIMSK = TIMSK & ~(1 << OCIE1B);
    // Disable capture interrupt.
    TIMSK = TIMSK & ~(1 << TICIE1);

    // Have received an interrupt so return the source.
    return OTHER_INT;
}
*/


/* A function to return the time of the last communications interrupt.  */
/* Must be called within 4ms of the last interrupt.  */
time_t
time_rf_timestamp_get (void)
{
    time_t timestamp;

    timestamp.us_ticks = ICR1;
    if (TCNT1 > ICR1)
        timestamp.ms_ticks = ms_ticks;
    else
        timestamp.ms_ticks = ms_ticks - 1;

    return timestamp;
}


/* A function to return the current time.  */
time_t
time_current_time_get (void)
{
    /* This is susceptible to rollover errors - FIXME.  */
    time_t timestamp;
    timestamp.us_ticks = TCNT1;
    timestamp.ms_ticks = ms_ticks;

    return timestamp;
}


/* A function to convert time_t variables into an unsigned int.  */
uint32_t
time_time2int (time_t time)
{
    return (uint32_t)(((uint32_t)time.ms_ticks * TOPCNT) + time.us_ticks);
}


void
time_irq_enable (void)
{
    TIMSK |= (1 << OCIE1A);
}


void
time_irq_disable (void)
{
    TIMSK &= ~(1 << OCIE1A);
}



// This increments the msTicks
SIGNAL (SIG_OUTPUT_COMPARE1A)
{
    ms_ticks++;
}


/* This interrupt is currently not used.  */
/*
// Just record the source of the interrupt
SIGNAL (SIG_OUTPUT_COMPARE1B)
{
    compare_flag = 1;
}
*/

/* This interrupt is used, but do not need to write data_flag.  */
SIGNAL (SIG_INPUT_CAPTURE1)
{
    // Copy the 16-bit value of Timer 1 to the global timestamp variable.
    // Compiler handles 16-bit access. 

    //data_flag = 1;
}


/** @file   time.h
    @author Ian Downes, UCECE
    @date   12 January 2005
    @brief  Time routines.
*/

#ifndef TIME_H
#define TIME_H

#include "config.h"
#include "irq.h"

#define TOPCNT (64000 - 1)
#define DELAY_MIN 10
#define DELAY_MAX 3999

typedef struct time
{
  uint16_t us_ticks;
  uint16_t ms_ticks;
} time_t;

typedef enum
{ ERRD, USTICKS, TIMEOUT, COMMS_INT, OTHER_INT } delay_ret_t;

extern void time_init (void);

extern time_t time_rf_timestamp_get (void);

extern time_t time_current_time_get (void);

//extern delay_ret_t time_delay_us (uint16_t delay_us);
extern void time_delay_us (uint16_t us);

extern uint32_t time_time2int (time_t time);

extern void time_irq_enable (void);

extern void time_irq_disable (void);

#endif

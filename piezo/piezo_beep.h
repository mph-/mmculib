/** @file   piezo_beep.h
    @author M. P. Hayes, UCECE
    @date   12 April 2007
    @brief  Piezo beeping routines.  Note these block.
*/

#ifndef PIEZO_BEEP_H
#define PIEZO_BEEP_H

#include "config.h"
#include "piezo.h"


/* Time in milliseconds for a short beep. */
#ifndef PIEZO_BEEP_SHORT_PERIOD_MS
#define PIEZO_BEEP_SHORT_PERIOD_MS 30
#endif

/* Time in milliseconds for a long beep. */
#ifndef PIEZO_BEEP_LONG_PERIOD_MS
#define PIEZO_BEEP_LONG_PERIOD_MS 200
#endif

/* Beep frequency in kHz.  */
#ifndef PIEZO_BEEP_FREQ_KHZ
#define PIEZO_BEEP_FREQ_KHZ 2
#endif


extern void
piezo_beep (piezo_t dev, uint16_t duration_ms);

extern void
piezo_beep_short (piezo_t dev);

extern void
piezo_beep_long (piezo_t dev);
        
#endif

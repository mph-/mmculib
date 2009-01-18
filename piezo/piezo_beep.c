/** @file   piezo_beep.c
    @author M. P. Hayes, UCECE
    @date   12 April 2007
    @brief  Piezo beeping routines (blocking).
*/

#include "piezo_beep.h"


/** Generate a beep,  */
void piezo_beep (piezo_t piezo, uint16_t duration_ms)
{
    uint16_t i;    

    for (i = 0; i < duration_ms * PIEZO_BEEP_FREQ_KHZ * 2; i++)
    {    
        port_pins_toggle (piezo->port, piezo->bitmask);   
        DELAY_US (500 / PIEZO_BEEP_FREQ_KHZ);  
    } 
}


/** Generate a short beep,  */
void piezo_beep_short (piezo_t piezo)
{
    uint16_t i;    

    for (i = 0; i < PIEZO_BEEP_SHORT_PERIOD_MS * PIEZO_BEEP_FREQ_KHZ * 2; i++)
    {    
        port_pins_toggle (piezo->port, piezo->bitmask);   
        DELAY_US (500 / PIEZO_BEEP_FREQ_KHZ);  
    } 
}


/** Generate a long beep,  */
void piezo_beep_long (piezo_t piezo)
{
    uint16_t i;    

    for (i = 0; i < PIEZO_BEEP_LONG_PERIOD_MS * PIEZO_BEEP_FREQ_KHZ * 2; i++)
    {    
        port_pins_toggle (piezo->port, piezo->bitmask);   
        DELAY_US (500 / PIEZO_BEEP_FREQ_KHZ);  
    } 
}

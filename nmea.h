/** @file   nmea.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#ifndef NMEA_H
#define NMEA_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

#ifndef NMEA_BUFFER_SIZE
#define NMEA_BUFFER_SIZE 80
#endif

extern uint8_t
nmea_checksum (const char *string);

extern
void nmea_puts (const char *string);



#ifdef __cplusplus
}
#endif    
#endif


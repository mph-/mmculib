/** @file   dscrc16.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#ifndef DSCRC16_H
#define DSCRC16_H

#ifdef __cplusplus
extern "C" {
#endif
    

#ifndef CRC161520_H
#define CRC161520_H

#include "config.h"

typedef uint16_t crc16_t;

/* Update crc for given byte VAL.  */
extern crc16_t dscrc16_byte (crc16_t crc, uint8_t val);

/* Update crc for SIZE bytes.  */
extern crc16_t dscrc16 (crc16_t crc, void *bytes, uint8_t size);

#endif

#ifdef __cplusplus
}
#endif    
#endif /* DSCRC16_H  */


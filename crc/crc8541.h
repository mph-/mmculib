/** @file   crc8541.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief 
*/
#ifndef CRC8541_H
#define CRC8541_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

typedef uint8_t crc8_t;

/* Update crc for given byte VAL.  */
extern crc8_t crc8541_byte (crc8_t crc, uint8_t val);

/* Update crc for SIZE BYTES.  */
extern crc8_t crc8541 (crc8_t crc, uint8_t *bytes, uint8_t size);


#ifdef __cplusplus
}
#endif    
#endif


/** @file   dscrc8.h
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  8-bit CRC routines for Dallas Semiconductor one wire bus.
*/
#ifndef DSCRC8_H
#define DSCRC8_H

#ifdef __cplusplus
extern "C" {
#endif
    

#ifndef CRC8540_H
#define CRC8540_H

#include "config.h"

typedef uint8_t crc8_t;


/** Update CRC for a single byte. 
    @param crc CRC value
    @param val value to update CRC with 
    @return new CRC  */
extern crc8_t dscrc8_byte (crc8_t crc, uint8_t val);


/** Update CRC for a number of bytes. 
    @param crc CRC value
    @param bytes pointer to array of bytes
    @param size number of bytes to process
    @return new CRC  */
extern crc8_t dscrc8 (crc8_t crc, uint8_t *bytes, uint8_t size);

#endif

#ifdef __cplusplus
}
#endif    
#endif /* DSCRC8_H  */


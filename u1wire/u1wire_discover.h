/** @file   u1wire_discover.h
    @author M. P. Hayes, UCECE
    @date   24 February 2005
    @brief  Discover devices on a Dallas universal one wire bus.
    @note   This has been superseded by u1wire_enumerate.
*/
#ifndef U1WIRE_DISCOVER_H
#define U1WIRE_DISCOVER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "u1wire.h"

int8_t
u1wire_discover (u1wire_obj_t *devices, uint8_t devices_max);


#ifdef __cplusplus
}
#endif    
#endif


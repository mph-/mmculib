/** @file   u1wire_enumerate.h
    @author M. P. Hayes, UCECE
    @date   24 February 2005
    @brief  This discovers devices on a Dallas universal one wire bus. 
*/
#ifndef U1WIRE_ENUMERATE_H
#define U1WIRE_ENUMERATE_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "u1wire.h"

typedef struct
{
    int8_t last_discrepancy;
    int8_t last_device;
    int8_t last_family_discrepancy;
} u1wire_state_t;


typedef struct
{
    u1wire_obj_t device;
    u1wire_state_t state;
} u1wire_enumerate_t;


u1wire_obj_t *
u1wire_enumerate (u1wire_enumerate_t *info);


u1wire_obj_t *
u1wire_enumerate_next (u1wire_enumerate_t *info);


/* Here's an example of how to enumerate all the devices
   on a Dallas universal one wire bus.
u1wire_enumerate_t info;
u1wire_obj_t *device;

for (device = u1wire_enumerate (&info); device; 
     device = u1wire_enumerate_next (&info)
    {


    }
*/



#ifdef __cplusplus
}
#endif    
#endif


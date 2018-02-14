/** @file   mu1wire.h
    @author M. P. Hayes, UCECE
    @date   08 June 2002
    @brief 
*/
#ifndef MU1WIRE_H
#define MU1WIRE_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

enum
{
    MU1WIRE_ERR_BUS_LOW = 1,     /* Can't drive bus high.  */
    MU1WIRE_ERR_BUS_STUCK = 2,   /* Bus stuck low.  */
    MU1WIRE_ERR_PRESENCE_SHORT = 3,      /* Slave presence pulse too short.  */
    MU1WIRE_ERR_PRESENCE_LONG = 4,        /* Slave presence pulse too long.  */
    MU1WIRE_ERR_MULTIPLE_DEVICES = 5,
    MU1WIRE_ERR_BUS_HIGH = 6     /* Can't drive bus low.  */
};

typedef union
{
    struct
    {
        uint8_t family;
        uint8_t serial[6];
        uint8_t crc;
    } fields;
    uint8_t bytes[8];
}
mu1wire_rom_code_t;


typedef struct
{
    mu1wire_rom_code_t rom_code;
} mu1wire_obj_t;

typedef mu1wire_obj_t * mu1wire_t;


/* This interface only supports the one bus but with multiple devices
  on the bus.  This was a deliberate choice that I may regret one day.
  It keeps the interface simpler, reduces the code size, and most
  systems will only ever have a single one wire bus.  To support multiple
  one wire buses then run-time configuration will need to be passed
  as an additional argument to mu1wire_init.  */


/* Return true if one wire bus operation has completed.   */
extern bool
mu1wire_ready_p (void);

extern int8_t
mu1wire_reset (void);

extern void
mu1wire_bit_write (uint8_t value);

extern void
mu1wire_byte_write (uint8_t value);

extern uint8_t
mu1wire_bit_read (void);

extern uint8_t
mu1wire_byte_read (void);

/* Send COMMAND to specified DEV.  */
extern int8_t
mu1wire_command (mu1wire_t dev, uint8_t command);

extern int8_t
mu1wire_broadcast (uint8_t command);

/* Read SIZE bytes of DATA.  */
extern int8_t
mu1wire_read (mu1wire_t dev, void *data, uint8_t size);

/* Write SIZE bytes of DATA.  */
extern int8_t
mu1wire_write (mu1wire_t dev, void *data, uint8_t size);

/* Initialise one wire bus and discover all the one wire devices on
   the bus up to a maximum of DEVICES_MAX devices.  This returns the
   number of found devices.  */
extern int8_t
mu1wire_init (mu1wire_obj_t *devices, uint8_t devices_max);

/* In mu1wire_debug.c  */
extern void
mu1wire_debug (mu1wire_t dev);


#ifdef __cplusplus
}
#endif    
#endif


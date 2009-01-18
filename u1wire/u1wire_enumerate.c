/** @file   u1wire_enumerate.c
    @author M. P. Hayes, UCECE
    @date   15 May 2007
    @brief  Discover and enumerate devices on a Dallas universal one wire bus.
    @note   This supersedes u1wire_discover.
*/
#include "dscrc8.h"
#include "u1wire.h"
#include "u1wire_enumerate.h"


enum
{ 
    U1WIRE_SEARCH = 0xf0
};


/* See Dallas application note AN187.  */

/* Read on the bus the logical and of all bits being transmitted.  */

/*
Bit (true)  Bit (complement) Information Known
0           0 Discrepancy
0           1 ROM code bit 0
1           0 ROM code bit 1
1           1 No devices participating in search
*/
 
static bool 
u1wire_search (u1wire_state_t *state, u1wire_rom_code_t *rom_code)
{
    int8_t id_bit_number;
    int8_t last_zero;
    int8_t rom_byte_number;
    int8_t id_bit;
    int8_t cmp_id_bit;
    uint8_t rom_byte_mask;
    uint8_t search_direction;
    uint8_t rom_byte;
    crc8_t crc;
 
    if (state->last_device) 
        return 0;

    id_bit_number = 1;
    last_zero = 0;
    rom_byte_number = 0;
    rom_byte_mask = 1;
    crc = 0;
    rom_byte = 0;

    /* 1-Wire reset.  */
    if (u1wire_reset () <= 0) 
        return 0;
    
    /* Broadcast search command.  */
    u1wire_byte_write (U1WIRE_SEARCH);
    
    do
    { 
        /*  Read a bit and its complement.  */
        id_bit = u1wire_bit_read ();
        cmp_id_bit = u1wire_bit_read ();
        
        /* Check for no devices.  */
        if (id_bit && cmp_id_bit)
            return 0;
        
        if (id_bit != cmp_id_bit)
        {
            /* All devices have a 0 or 1 bit.  */
            search_direction = id_bit;
        }
        else
        { 
            /* Have a discrepancy.  If this discrepancy is before the
              last discrepancy on a previous next then pick the same
              as last time.  */
            if (id_bit_number < state->last_discrepancy) 
                search_direction = (rom_code->bytes[rom_byte_number]
                                    & rom_byte_mask) > 0;
            else 
                /* If equal to last pick 1, if not then pick 0.  */
                search_direction = (id_bit_number == state->last_discrepancy);
            
            /*  If 0 was picked then record its position in LastZero.  */
            if (search_direction == 0) 
            {
                last_zero = id_bit_number;
                
                /*  Check for last discrepancy in family.  */
                if (last_zero < 9)
                    state->last_family_discrepancy = last_zero;
            } 
        } 
        
        /* Set bit in the rom code.  */
        if (search_direction == 1)
            rom_byte |= rom_byte_mask;
        
        /* Write serial number search direction.  */
        u1wire_bit_write (search_direction);
        
        id_bit_number++;
        rom_byte_mask <<= 1;
        
        if (rom_byte_mask == 0)
        { 
            crc = dscrc8_byte (crc, rom_byte);
            rom_code->bytes[rom_byte_number] = rom_byte;
            rom_byte_number++;
            rom_byte_mask = 1;
            rom_byte = 0;
        }
    }
    while (rom_byte_number < 8);
    
    
    if ((id_bit_number < 65) || crc || !rom_code->bytes[0])
        return 0;

    state->last_discrepancy = last_zero;
        
    /* Check for last device.  */
    if (state->last_discrepancy == 0) 
        state->last_device = 1;

    return 1;
}


u1wire_obj_t *
u1wire_enumerate_next (u1wire_enumerate_t *info)
{
    if (! u1wire_search (&info->state, &info->device.rom_code))
        return 0;

    return &info->device;
}


u1wire_obj_t *
u1wire_enumerate (u1wire_enumerate_t *info)
{
    info->state.last_discrepancy = 0;
    info->state.last_device = 0;
    info->state.last_family_discrepancy = 0;

    return u1wire_enumerate_next (info);
}



/* File:   qdecoder.h
   Author: M. P. Hayes, UCECE
   Date:   26 June 2002
   Descr: 
*/
#ifndef QDECODER_H
#define QDECODER_H

#include "config.h"
#include "port.h"

typedef int16_t qdecoder_pos_t;


#define QDECODER_CFG(PORT0, PORTBIT0, PORT1, PORTBIT1) \
    {(PORT0), BIT (PORTBIT0), (PORT1), BIT (PORTBIT1)}


/* Qdecoder configuration structure.  */
typedef struct
{
    port_t port0;
    port_mask_t bitmask0;
    port_t port1;
    port_mask_t bitmask1;
} qdecoder_cfg_t;


typedef struct
{
    uint8_t state;
    uint8_t errs;
    qdecoder_pos_t pos;
    const qdecoder_cfg_t *cfg;
} qdecoder_private_t;


typedef qdecoder_private_t qdecoder_obj_t;
typedef qdecoder_obj_t *qdecoder_t;


static inline uint8_t
qdecoder_state (qdecoder_t this)   				
{
    uint8_t state;						

    state = 0;

    if (port_pins_read (this->cfg->port1, this->cfg->bitmask1))
        state = 2;

    if (port_pins_read (this->cfg->port0, this->cfg->bitmask0))
        state++;
    
    return state;
}


/* Make this inlined since often called from an ISR.  */
static inline void
qdecoder_poll (qdecoder_t this)   				
{								
    uint8_t state;						

    /* This can be constructed as a Mealy state machine with 4 states
       and 4 outputs: no change, increment, decrement, and error.  */

    state = qdecoder_state (this);

    switch (state ^ this->state)
    {				
    case 1:
    case 2:
        if ((this->state >> 1) ^ (state & 1))
            this->pos--;
        else
            this->pos++;            
        break;

    case 3:
        /* If jump two transitions then have an error.  
           This is due to not sampling fast enough.  */
        this->errs++;        
        break;

    default:
        return;
    }
    this->state = state;
}								


static inline uint8_t
qdecoder_errs_get (qdecoder_t this)
{
    uint8_t errs;

    errs = this->errs;
    this->errs = 0;

    return errs;
}


static inline qdecoder_pos_t
qdecoder_pos_get (qdecoder_t this)
{
    return this->pos;
}


static inline void
qdecoder_pos_set (qdecoder_t this, qdecoder_pos_t pos)
{
    this->pos = pos;
}


/* Create a new quadrature decoder device.  */
extern qdecoder_t
qdecoder_init (qdecoder_obj_t *this, const qdecoder_cfg_t *cfg);



#endif /* QDECODER_H  */

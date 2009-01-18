/* File:   qdecoder.c
   Author: M. P. Hayes, UCECE
   Date:   26 June 2002
   Descr:  Quadrature decoder
*/
#include "qdecoder.h"
#include "port.h"

/* Create a new quadrature decoder device.  */
qdecoder_t
qdecoder_init (qdecoder_obj_t *this, const qdecoder_cfg_t *cfg)
{
    /* Configure ports as inputs.  */
    port_pins_config_pullup (cfg->port1, cfg->bitmask1);
    port_pins_config_pullup (cfg->port0, cfg->bitmask0);
    
    this->cfg = cfg;

    this->state = qdecoder_state (this);    
    this->pos = 0;
    this->errs = 0;
    
    return this;
}

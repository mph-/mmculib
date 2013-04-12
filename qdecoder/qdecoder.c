/* File:   qdecoder.c
   Author: M. P. Hayes, UCECE
   Date:   26 June 2002
   Descr:  Quadrature decoder
*/
#include "qdecoder.h"
#include "pio.h"

/* Create a new quadrature decoder device.  */
qdecoder_t
qdecoder_init (qdecoder_obj_t *this, const qdecoder_cfg_t *cfg)
{
    /* Configure ports as inputs.  */
    pio_config_set(cfg->pio1, PIO_PULLUP);
    pio_config_set(cfg->pio2, PIO_PULLUP);
    
    this->cfg = cfg;

    this->state = qdecoder_state (this);    
    this->pos = 0;
    this->errs = 0;
    
    return this;
}

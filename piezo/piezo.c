/** @file   piezo.c
    @author M. P. Hayes, UCECE
    @date   12 March 2003
    @brief 
*/
   
#include "piezo.h"


/* Create a new PIEZO device.  */
piezo_t
piezo_init (const piezo_cfg_t *cfg)
{
    /* Configure pio as output.  */
    pio_config_set (cfg->pio, PIO_OUTPUT_LOW);

    return cfg;
}

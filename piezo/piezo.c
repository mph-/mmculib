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
    /* Configure port as output.  */
    port_pins_config_output (cfg->port, cfg->bitmask);

    return cfg;
}

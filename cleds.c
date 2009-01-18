/** @file   cleds.c
    @author M. P. Hayes, UCECE
    @date   29 June 2007
    @brief 
*/

#include "cleds.h"
#include "port.h"

void
cleds_init (cleds_obj_t *cleds,
            const led_cfg_t *row_config, uint8_t rows_num,
            const led_cfg_t *col_config, uint8_t cols_num)
{
    uint8_t i;
    
    cleds->leds = col_config;
    cleds->row_config = row_config;
    cleds->rows_num = rows_num;
    cleds->cols_num = cols_num;

    /* Make row drivers outputs and set low.  */
    for (i = 0; i < rows_num; i++)
    {
        port_pins_config_output (row_config->port, row_config->bitmask);
        port_pins_set_low (row_config->port, row_config->bitmask);
        row_config++;
    }

    /* Initialise leds.  */
    for (i = 0; i < cols_num; i++)
        led_init (col_config++);

    cleds->row = 0;
}


uint8_t
cleds_common_set (cleds_t cleds, uint8_t row)
{
    uint8_t oldrow;
    const led_cfg_t *row_config = cleds->row_config;    

    oldrow = cleds->row;

    if (row == oldrow)
        return oldrow;

    if (oldrow)
        port_pins_set_low (row_config[oldrow - 1].port,
                           row_config[oldrow - 1].bitmask);

    if (row)
        port_pins_set_high (row_config[row - 1].port,
                            row_config[row - 1].bitmask);
    cleds->row = row;
    return oldrow;
}

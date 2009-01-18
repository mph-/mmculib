/** @file   mbuttons.c
    @author M. P. Hayes, UCECE
    @date   29 June 2007
    @brief 
*/

#include "mbuttons.h"
#include "port.h"

void
mbuttons_init (mbuttons_obj_t *mbuttons,
               button_obj_t *mbutton_objs, 
               const button_cfg_t *row_config, uint8_t rows_num,
               const button_cfg_t *col_config, uint8_t cols_num,
               uint8_t poll_count)
{
    uint8_t i;
    uint8_t j;
    button_t buttons = mbutton_objs;
    
    mbuttons->buttons = buttons;
    mbuttons->row_config = row_config;
    mbuttons->rows_num = rows_num;
    mbuttons->cols_num = cols_num;

    for (i = 0; i < rows_num; i++)
    {
        /* Make row driver output and set low.  */
        port_pins_config_output (row_config[i].port, row_config[i].bitmask);
        port_pins_set_low (row_config[i].port, row_config[i].bitmask);

        /* Initialise each button on this row.  */
        for (j = 0; j < cols_num; j++)
            button_init (buttons++, col_config + j);        
    }
    
    button_poll_count_set (poll_count);
}


void
mbuttons_poll (mbuttons_t mbuttons)
{
    uint8_t i;
    uint8_t j;
    button_t buttons = mbuttons->buttons;   
    const button_cfg_t *cfg;

    /* Set all rows high.  */
    cfg = mbuttons->row_config;
    for (i = 0; i < mbuttons->rows_num; i++, cfg++)
        port_pins_set_high (cfg->port, cfg->bitmask);           

    cfg = mbuttons->row_config;
    for (i = 0; i < mbuttons->rows_num; i++, cfg++)
    {
        /* Set selected row low.  */
        port_pins_toggle (cfg->port, cfg->bitmask);     

        /* Poll buttons on current row.  */
        for (j = 0; j < mbuttons->cols_num; j++)
            button_poll (buttons++);

        /* Set selected row high again.  */
        port_pins_toggle (cfg->port, cfg->bitmask);
    }
}


bool
mbuttons_any_state_p (mbuttons_t mbuttons, button_state_t state)
{
    uint8_t i;
    button_t buttons = mbuttons->buttons;

    for (i = 0; i < mbuttons->rows_num * mbuttons->cols_num; i++)
        if (button_state_get (buttons++) == state)
            return 1;

    return 0;
}


void
mbuttons_wakeup_init (mbuttons_t mbuttons)
{
    uint8_t i;
    const button_cfg_t *cfg;

    /* Set all rows low.  */
    cfg = mbuttons->row_config;
    for (i = 0; i < mbuttons->rows_num; i++, cfg++)
        port_pins_set_low (cfg->port, cfg->bitmask);            
}

/** @file   lmatrix.c
    @author M. P. Hayes, UCECE
    @date   28 March 2007
    @brief  Drive a multiplexed LED matrix.
    @note   This only supports a single instance.
*/

#include "lmatrix.h"
#include "port.h"


#ifdef LMATRIX_ROWS_SWAP
#define ROWBIT(N) BIT(LMATRIX_ROWS - 1 - (N))
#else
#define ROWBIT(N) BIT(N)
#endif



/* The columns are numbered from left to right while the rows
   are numbered from top to bottom.  */

static lmatrix_port_t col_ports[] = 
{
    {LMATRIX_COL1_PORT, BIT (LMATRIX_COL1_BIT)},
    {LMATRIX_COL2_PORT, BIT (LMATRIX_COL2_BIT)},
    {LMATRIX_COL3_PORT, BIT (LMATRIX_COL3_BIT)},
    {LMATRIX_COL4_PORT, BIT (LMATRIX_COL4_BIT)},
    {LMATRIX_COL5_PORT, BIT (LMATRIX_COL5_BIT)}
};


void lmatrix_update (lmatrix_t lmatrix)
{
    lmatrix_port_t *col_port = lmatrix->col_port;

    /* Deactivate last column.  */
    port_pins_set_high (col_port->port, col_port->bitmask);

    col_port++;
    lmatrix->col++;
    if (lmatrix->col >= LMATRIX_COLS)
    {
        lmatrix->col = 0;
        col_port = col_ports;
    }

    port_bus_write (LMATRIX_ROW_PORT,
                    LMATRIX_ROW_BIT_FIRST,
                    LMATRIX_ROW_BIT_LAST,
                    lmatrix->state[lmatrix->col]);

    /* Activate next column.  */
    port_pins_set_low (col_port->port, col_port->bitmask);
    lmatrix->col_port = col_port;
}


void
lmatrix_set (lmatrix_t lmatrix, uint8_t row, uint8_t col, bool val)
{
    uint8_t bitmask;
    uint8_t state;

    bitmask = ROWBIT (row);

    /* Set bit to turn pixel off then toggle it if we want it
       on.  */
    state = lmatrix->state[col] |= bitmask;
    if (val)
        state ^= bitmask;

    lmatrix->state[col] = state;
}


#ifdef LMATRIX_ROWS_SWAP
void
lmatrix_write (lmatrix_t lmatrix, uint8_t *screen, uint8_t *map)
{
    uint8_t bitmask;
    lmatrix_row_state_t *states = lmatrix->state;

    for (bitmask = ROWBIT (0); bitmask; bitmask >>= 1)
    {
        uint8_t col;

        for (col = 0; col < LMATRIX_COLS; col++)
        {
            uint8_t state = states[col] |= bitmask;

            if (map[*screen++])
                state ^= bitmask;
            
            states[col] = state;
        }
    }
}

#else

void
lmatrix_write (lmatrix_t lmatrix, uint8_t *screen, uint8_t *map)
{
    uint8_t row;
    uint8_t bitmask = ROWBIT (0);

    for (row = 0; row < LMATRIX_ROWS; row++)
    {
        uint8_t col;

        for (col = 0; col < LMATRIX_COLS; col++)
        {
            uint8_t state = lmatrix->state[col] |= bitmask;

            if (map[*screen++])
                state ^= bitmask;
            
            lmatrix->state[col] = state;
        }
        bitmask <<= 1;
    }
}

#endif



lmatrix_t
lmatrix_init (lmatrix_obj_t *lmatrix)
{
    uint8_t i;

    port_bus_config_output (LMATRIX_ROW_PORT,
                            LMATRIX_ROW_BIT_FIRST,
                            LMATRIX_ROW_BIT_LAST);
    port_bus_write (LMATRIX_ROW_PORT,
                    LMATRIX_ROW_BIT_FIRST, LMATRIX_ROW_BIT_LAST, ~0);
    
    for (i = 0; i < LMATRIX_COLS; i++)
    {
        port_pins_config_output (col_ports[i].port, col_ports[i].bitmask);
        port_pins_set_high (col_ports[i].port, col_ports[i].bitmask);
        lmatrix->state[i] = ~0;
    }
    lmatrix->col_port = col_ports;
    lmatrix->col = 0;

    return lmatrix;
}

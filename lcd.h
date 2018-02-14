/** @file   lcd.h
    @author M. P. Hayes, UCECE
    @date   15 Feb 2003
    @brief 
*/
#ifndef LCD_H
#define LCD_H

#ifdef __cplusplus
extern "C" {
#endif
    


#include "config.h"
#include "port.h"


typedef struct
{
    port_t data_port;
    /* The 4 data bits must be contiguous with the
       least significant bit (D4) specified by d_bit.  */
    port_bit_t d_bit;
    port_t e_port;
    port_bit_t e_bit;
    port_t rs_port;
    port_bit_t rs_bit;
} lcd_cfg_t;

typedef struct
{
    const lcd_cfg_t *cfg;
    port_mask_t e_mask;
    port_mask_t rs_mask;
    uint8_t data;
} lcd_obj_t;      
      

typedef lcd_obj_t *lcd_t;

extern void lcd_putc (lcd_t dev, char);

extern void lcd_puts (lcd_t dev, const char *);

extern void lcd_clear (lcd_t dev);

extern void lcd_goto (lcd_t dev, uint8_t row, uint8_t col);

extern lcd_t lcd_init (lcd_obj_t *info, const lcd_cfg_t *cfg);


#ifdef __cplusplus
}
#endif    
#endif


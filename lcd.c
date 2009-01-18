/** @file   lcd.c
    @author M.P. Hayes
    @date   16 Feb 2003

    @brief Routines to interface to a Hitachi HD44780/KS0066
   LCD controller in 4-bit mode with no readback.  
*/

#include "config.h"
#include "lcd.h"
#include "delay.h"

#define LCD_DEBUG HOSTED

#if LCD_DEBUG
#include <stdio.h>
#endif


/* This modules assumes that the LCD controller is interfaced
   in 4-bit mode with all the signals connected to the same
   port.  If the R/W signal is not used, the LCD controller
   cannot be polled to determine when a command has completed
   and thus we have to wait for a fixed delay.  */

enum {LCD_CLEAR = BIT (0),
      LCD_HOME = BIT (1),
      LCD_ENTRY_MODE = BIT (2),
      LCD_DISPLAY = BIT (3),
      LCD_SHIFT = BIT (4),
      LCD_FUNCTION = BIT (5),
      LCD_CG_RAM_ADDRESS = BIT (6),
      LCD_DD_RAM_ADDRESS = BIT (7)};


static const uint8_t lcd_init_data[] =
{
    /* 4 bit mode, 1/16 duty, (F=1) 5x10 font, (N=1) 2 lines.  */
    LCD_FUNCTION | BIT (3) | BIT (2),
    /* Display on.  */
    LCD_DISPLAY | BIT (2),
    /* Entry mode advance cursor.  */
    LCD_ENTRY_MODE | BIT (1),
    /* Clear display and reset cursor (this takes up to 2 ms).  */
    LCD_CLEAR
};



#ifdef LCD_RW
/* Poll LCD waiting for status bit to be asserted.  */
#error Polling of LCD not supported
#else
/* Wait until command completed.  */
#define lcd_wait(dev) DELAY_US (40 + 20)
#endif


/* Drive data bus.  Assume D4, D5, D6, D7 are contiguous.  */
#define lcd_data_set(dev, data)                                 \
    port_bus_write (dev->cfg->data_port,                        \
                    dev->cfg->d_bit, dev->cfg->d_bit + 3, (data))


/* Select control register.  */
#define lcd_mode_control(dev) \
    port_pins_set_low (dev->cfg->rs_port, dev->rs_mask)


/* Select data register.  */
#define lcd_mode_data(dev) \
    port_pins_set_high (dev->cfg->rs_port, dev->rs_mask)


/* Strobe enable signal.  */
#define lcd_strobe(dev)                                         \
do                                                              \
{                                                               \
    port_pins_set_high (dev->cfg->e_port, dev->e_mask);                 \
    DELAY_US (2);                                               \
    port_pins_set_low (dev->cfg->e_port, dev->e_mask);                  \
}                                                               \
while (0) 


/* Write command/data char to LCD.  */
#if 1
#define lcd_write(dev, data)                                    \
do                                                              \
{                                                               \
    uint8_t _tmp = (data);                                      \
                                                                \
    /* Send MS nibble.  */                                      \
    lcd_data_set (dev, _tmp >> 4);                              \
    lcd_strobe (dev);                                           \
    /* Send LS nibble.  */                                      \
    lcd_data_set (dev, _tmp);                                   \
    lcd_strobe (dev);                                           \
    lcd_wait (dev);                                             \
}                                                               \
while (0) 

#else

static void
lcd_write (lcd_t dev, uint8_t data)
{
    /* Send MS nibble.  */
    lcd_data_set (dev, data << 4);
    lcd_strobe (dev);
    /* Send LS nibble.  */
    lcd_data_set (dev, (data));
    lcd_strobe (dev);
    lcd_wait (dev);
}
#endif


/* Write char to LCD.  */
void 
lcd_putc (lcd_t dev, char ch)
{
#if LCD_DEBUG
    fputc (ch, stderr);
#endif
    if (ch == '\n')
    {
        lcd_mode_control (dev);
        lcd_write (dev, LCD_DD_RAM_ADDRESS | (1 << 6));
    }
    else
    {
        lcd_mode_data (dev);
        lcd_write (dev, ch);
    }
}


/* Write string to LCD.  */
void 
lcd_puts (lcd_t dev, const char *str)
{
#if LCD_DEBUG
    fprintf (stderr, "<%s>\n", str);
#endif
    while (*str)
    {
        lcd_putc (dev, *str++);
    }
}


/* Clear LCD.  */
void
lcd_clear (lcd_t dev)
{
    lcd_mode_control (dev);
    lcd_write (dev, LCD_CLEAR);
    delay_ms (2 + 1);

    /* Set data pins to 0 to avoid powering LCD
       through data lines when LCD power removed.  */
    lcd_data_set (dev, 0);
}


/* Move to position (row, col) on LCD.  */
void 
lcd_goto (lcd_t dev, uint8_t row, uint8_t col)
{
    lcd_mode_control (dev);
    lcd_write (dev, LCD_DD_RAM_ADDRESS | (row << 6) | (col & 0x2f));
}


/* Create a new LCD device.  */
lcd_t
lcd_init (lcd_obj_t *info, const lcd_cfg_t *cfg)
{
    lcd_t dev = (lcd_t)info;
    uint8_t i;

    info->cfg = cfg;
    info->e_mask = BIT (cfg->e_bit);
    info->rs_mask = BIT (cfg->rs_bit);

    /* Configure port for desired signals.  */
    port_pin_config_output (cfg->data_port, cfg->d_bit);
    port_pin_config_output (cfg->data_port, cfg->d_bit + 1);
    port_pin_config_output (cfg->data_port, cfg->d_bit + 2);
    port_pin_config_output (cfg->data_port, cfg->d_bit + 3);
    port_pin_config_output (cfg->rs_port, cfg->rs_bit);
    port_pin_config_output (cfg->e_port, cfg->e_bit);

    lcd_mode_control (dev);

    /* Power on delay.  */
    delay_ms (15 + 5);

    lcd_data_set (dev, 0x03);
    lcd_strobe (dev);
    /* Require 4.1 ms.  */      
    delay_ms (5 + 2);
    
    lcd_strobe (dev);
    DELAY_US (100 + 5);
    
    lcd_strobe (dev);
    delay_ms (5 + 2);
    
    /* Set 4 bit mode.  */
    lcd_data_set (dev, 0x02);
    lcd_strobe (dev);   
    DELAY_US (40 + 60);
    
    /* Send init commands.  */
    for (i = 0; i < ARRAY_SIZE (lcd_init_data); i++)
        lcd_write (dev, lcd_init_data[i]);
    
    /* The delay is required for the clear command.  */
    delay_ms (2 + 1);
    return dev;
}

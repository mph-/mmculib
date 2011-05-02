/** @file   glcd.c
    @author M. P. Hayes, UCECE
    @author B. C. Bonnett, UCECE
    @date   4 November 2010
    @brief  Simple graphical GLCD driver.
*/

#include "config.h"
#include "glcd.h"
#include "delay.h"
#include "spi.h"

#include <stdlib.h>

/* This driver controls an S6B1713 GLCD controller via SPI.  The
   S6B1713 is a 65 com / 132 seg driver and controller for STN GLCD.

   The Displaytech 64128K uses the Sitronix S77565 controller
   with 1/65 duty and 1/9 bias. 

   The maximum clock speed is 20 MHz at 3.3 V, 10 MHz at 2.7 V.

   The CS setup time is 55 ns at 3.3 V.
   The CS hold time is 180 ns at 3.3 V.
*/


#ifndef GLCD_SPI_CLOCK_SPEED_KHZ
#define GLCD_SPI_CLOCK_SPEED_KHZ 20000
#endif


/* The following macros must be defined:

   GLCD_SPI_CHANNEL if the SPI controller supports multiple channels,
   GLCD_CS to specify the PIO controlling the CS signal,
   GLCD_RS to specify the PIO controlling the RS signal.

   The following macros are optional:

   GLCD_RESET to specify the PIO if the reset signal is connected.
   GLCD_BACKLIGHT to specify the PIO if the backlight is connected.
*/

enum
{
    GLCD_CMD_DISPLAY_OFF = 0xAE,
    GLCD_CMD_DISPLAY_ON = 0xAF,
    GLCD_CMD_REF_VOLTAGE_MODE = 0x81,
    GLCD_CMD_PAGE_ADDRESS_SET = 0xB0,
    GLCD_CMD_COL_ADDRESS_MSB_SET = 0x10,
    GLCD_CMD_COL_ADDRESS_LSB_SET = 0x00,
    GLCD_CMD_SEGOUTPUT_NORMAL = 0xA0,
    GLCD_CMD_SEGOUTPUT_REVERSE = 0xA1,
    GLCD_CMD_COMOUTPUT_NORMAL = 0xC0,
    GLCD_CMD_COMOUTPUT_REVERSE = 0xC8,
    GLCD_CMD_DISPLAY_NORMAL = 0xA6,
    GLCD_CMD_DISPLAY_REVERSE = 0xA7,
    GLCD_CMD_DISPLAY_ENTIRE_OFF = 0xA4,
    GLCD_CMD_DISPLAY_ENTIRE_ON = 0xA5,
    GLCD_CMD_BIAS0 = 0xA2,
    GLCD_CMD_BIAS1 = 0xA3,
    GLCD_CMD_MODIFY_READ_ON = 0xE0,
    GLCD_CMD_MODIFY_READ_OFF = 0xEE,
    GLCD_CMD_POWER = 0x28,
    GLCD_CMD_INITIAL_LINE_SET = 0x40,
    GLCD_CMD_REG_RESISTOR = 0x20,
    /* Reset the controller.  */
    GLCD_CMD_RESET = 0xE2,
};


/* Power control circuits.  Or the necessary GLCD_VOLT_* constants
   with GLCD_CMD_POWER, e.g., (GLCD_CMD_POWER | GLCD_VOLT_REGULATOR)
   will turn the regulator on and the converter and follower off.  */
#define GLCD_VOLT_CONVERTER                0x04
#define GLCD_VOLT_REGULATOR                0x02
#define GLCD_VOLT_FOLLOWER                 0x01

/* Set the internal voltage regulator resistor ratio 1 + (Rb / Ra)
   or with the desired value:
   000 = 1.90    001 = 2.19     010 = 2.55
   011 = 3.02    100 = 3.61     101 = 4.35
   110 = 5.29    111 = 6.48
*/



static inline void
glcd_command_mode (void)
{
    pio_output_low (GLCD_RS);
}


static inline void
glcd_data_mode (void)
{
    pio_output_high (GLCD_RS);
}


static inline void
glcd_send (glcd_t glcd, char ch)
{
    spi_putc (glcd->spi, ch);
}


void
glcd_backlight_enable (glcd_t glcd __UNUSED__)
{
#ifdef GLCD_BACKLIGHT
    pio_config_set (GLCD_BACKLIGHT, PIO_OUTPUT_HIGH);
    pio_output_high (GLCD_BACKLIGHT);
#endif
}


void
glcd_backlight_disable (glcd_t glcd __UNUSED__)
{
#ifdef GLCD_BACKLIGHT
    pio_config_set (GLCD_BACKLIGHT, PIO_OUTPUT_HIGH);
    pio_output_low (GLCD_BACKLIGHT);
#endif
}


static void
glcd_config (glcd_t glcd)
{
    /* Configure the RS pin as an output.  */
    pio_config_set (GLCD_RS, PIO_OUTPUT_HIGH);
    
    /* Enter command mode.  */
    glcd_command_mode ();

    /* Set segment lines to normal direction.  */
    glcd_send (glcd, GLCD_CMD_SEGOUTPUT_NORMAL);

    /* The Sitronix ST7565R used to drive the 64128K LCD on the 7C board needs
       the common lines reversed. */
#ifdef TREETAP7C
    glcd_send (glcd, GLCD_CMD_COMOUTPUT_REVERSE);
#else
    glcd_send (glcd, GLCD_CMD_COMOUTPUT_NORMAL);
#endif
    
    /* Set the GLCD bias to 1/9.  With a duty ratio of 1/65 the GLCD
       bias is 1/7 or 1/9.  */
#ifdef TREETAP7C
    glcd_send (glcd, GLCD_CMD_BIAS0);
#else
    glcd_send (glcd, GLCD_CMD_BIAS1);
#endif

    /* Bring the power circuits online in stages, pausing between each.  */
    glcd_send (glcd, GLCD_CMD_POWER | GLCD_VOLT_CONVERTER);
    delay_ms (1);

    glcd_send (glcd, GLCD_CMD_POWER | GLCD_VOLT_CONVERTER | GLCD_VOLT_REGULATOR);
    delay_ms (1);

    glcd_send (glcd, GLCD_CMD_POWER | GLCD_VOLT_CONVERTER 
              | GLCD_VOLT_REGULATOR | GLCD_VOLT_FOLLOWER);
    delay_ms (1);
    
    /* Set the reference voltage parameter alpha to 24 and the
       resistor regulator ratio to 4.35. 

       Note the charge pump boosts Vss to Vout and this then gets regulated
       for driving the LCD (9 V max).
       
       Vo = (1 + Rb / Ra) * Vev
       
       Vev = (1 - (63 - alpha) / 300) * Vref
       (The divisor appears to be 162 for the ST7565R).

       Now Vref = 2 V so with alpha = 24,
       Vev = (1 - (63 - 24) / 300) * 2 = 1.74 V
       Vo = 4.35 * 1.74 = 7.57 V
       
       require Vo < Vout (where Vout is set by external capacitor
       configuration)  */    
    glcd_send (glcd, GLCD_CMD_REF_VOLTAGE_MODE);
    /* Send alpha, range 0 to 63.  0 gives the greatest voltage for Vo.    */
    glcd_send (glcd, 50);
    /* Set Rb / Ra ratio to 5.  Can be in range 0 to 7.  */
    glcd_send (glcd, GLCD_CMD_REG_RESISTOR | 5);
    
    /* Set initial display line to zero.  */
    glcd_send (glcd, GLCD_CMD_INITIAL_LINE_SET | 0);

    glcd_send (glcd, GLCD_CMD_DISPLAY_NORMAL);
    
    /* Turn the GLCD on.  */
    glcd_send (glcd, GLCD_CMD_DISPLAY_ON);
}


void 
glcd_contrast_set (glcd_t glcd __UNUSED__, uint8_t contrast)
{
    int alpha;

#if 0
    alpha = (3 * contrast_percent) - 300 + 63;
#else
    alpha = contrast;
#endif
    if (alpha > 63)
        alpha = 63;

    glcd_command_mode ();
    glcd_send (glcd, GLCD_CMD_REF_VOLTAGE_MODE);
    glcd_send (glcd, alpha);
    glcd_send (glcd, GLCD_CMD_REG_RESISTOR | 5);
}


void
glcd_mode_set (glcd_t glcd, glcd_mode_t mode)
{
    glcd_command_mode ();
    if (mode == GLCD_MODE_INVERT)
        glcd_send (glcd, GLCD_CMD_DISPLAY_REVERSE);
    else
        glcd_send (glcd, GLCD_CMD_DISPLAY_NORMAL);
}


glcd_t
glcd_init (glcd_dev_t *dev, const glcd_cfg_t *cfg)
{
    glcd_t glcd;
    const spi_cfg_t spi_cfg = 
    {
        .channel = GLCD_SPI_CHANNEL,
        .clock_speed_kHz = GLCD_SPI_CLOCK_SPEED_KHZ,
        .cs = GLCD_CS,
        .mode = SPI_MODE_3,
        .bits = 8
    };

    glcd = dev;
    glcd->cfg = cfg;

    glcd->spi = spi_init (&spi_cfg);

    spi_cs_setup_set (glcd->spi, 2);
    spi_cs_hold_set (glcd->spi, 4);

#ifdef GLCD_RESET
    pio_config_set (GLCD_RESET, PIO_OUTPUT_HIGH);
    pio_output_low (GLCD_RESET);
    /* Minimum reset pulse is 1 us.  */
    DELAY_US (2);
    pio_output_high (GLCD_RESET);
    /* Need to wait 2 us for reset complete.  */
    DELAY_US (2);
#endif

    glcd_origin_set (glcd, 0, 0);
    glcd_config (glcd);

    glcd->modified = 0;
    glcd_clear (glcd);

    return glcd;
}


void 
glcd_origin_set (glcd_t glcd, uint16_t x, uint16_t y)
{
    glcd->xoff = x;
    glcd->yoff = y;
}


/* This has no effect until glcd_update called.  */
void 
glcd_pixel_set (glcd_t glcd, uint16_t x, uint16_t y, uint8_t val)
{
    uint8_t page;
    uint8_t col;
    uint8_t mask;
    uint8_t oldval;
    uint8_t newval;

    x += glcd->xoff;
    y += glcd->yoff;

    if (x >= GLCD_WIDTH || y >= GLCD_HEIGHT)
        return;

    col = x;
    page = y / GLCD_PAGE_PIXELS;
    
    mask = BIT (y % GLCD_PAGE_PIXELS);

    oldval = glcd->screen[page * GLCD_WIDTH + col];

    if (val)
        newval = oldval | mask;
    else
        newval = oldval & ~mask;

    glcd->screen[page * GLCD_WIDTH + col] = newval;

    /* Record which page modified.  */
    if (oldval != newval)
        glcd->modified |= BIT (page);
}


static void 
glcd_update_page (glcd_t glcd, uint8_t page)
{
    uint8_t commands[3];

    glcd_command_mode ();

    /* Move to start of selected page.  */
    commands[0] = GLCD_CMD_PAGE_ADDRESS_SET | page;
    commands[1] = GLCD_CMD_COL_ADDRESS_MSB_SET | 0;
    commands[2] = GLCD_CMD_COL_ADDRESS_LSB_SET | 0;
    spi_write (glcd->spi, commands, sizeof (commands), 1);

    DELAY_US (5);

    /* Write page.  */
    glcd_data_mode ();            
    spi_write (glcd->spi, &glcd->screen[page * GLCD_WIDTH],
               GLCD_WIDTH, 1);

    glcd->modified &= ~BIT (page);
}


void 
glcd_update (glcd_t glcd)
{
    uint8_t page;

    /* Transfer backing image to GLCD but only for pages modified.  */
    for (page = 0; glcd->modified && page < GLCD_PAGES; page++)
    {
        if (glcd->modified & BIT (page))
            glcd_update_page (glcd, page);
    }
}


void 
glcd_clear (glcd_t glcd)
{
    uint8_t page;
    uint8_t col;

    for (page = 0; page < GLCD_PAGES; page++)
    {
        for (col = 0; col < GLCD_WIDTH; col++)
        {
            glcd->screen[page * GLCD_WIDTH + col] = 0;
        }
        glcd->modified |= BIT (page);
    }

    glcd_update (glcd);
}


void
glcd_shutdown (glcd_t glcd)
{
    spi_shutdown (glcd->spi);

    pio_config_set (GLCD_CS, PIO_OUTPUT_HIGH);

    pio_output_low (GLCD_CS);
    pio_output_low (GLCD_RS);

#ifdef GLCD_RESET
    pio_output_low (GLCD_RESET);
#endif
}

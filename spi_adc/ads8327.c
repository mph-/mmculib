#include "config.h"
#include "pio.h"
#include "spi.h"
#include "spi_adc.h"


static inline void 
ads8327_chip_select (void)
{
   pio_output_low (SPI_ADC_CS_PIO);
}


static inline void 
ads8327_chip_deselect (void)
{
    pio_output_high (SPI_ADC_CS_PIO);
}


#if 0
void
ads8327_debug (void)
{
    uint8_t byte0;
    uint8_t byte1;
    uint16_t val;
    char buffer[16];

    ads8327_chip_select ();     

    byte0 = spi_xferc (0xc0);
    byte1 = spi_xferc (0x00);

    ads8327_chip_deselect ();

    val = (byte0 << 8) | byte1;
    
    //sprintf (buffer, "%d\n", val);
    uint16toa (val, buffer, 0);
    usart0_puts (buffer);
}
#endif


static inline
uint16_t ads8327_command (uint16_t val)
{
    uint8_t hi;
    uint8_t lo;

    ads8327_chip_select ();

    hi = spi_xferc (val >> 8);
    lo = spi_xferc (val & 0xff);

    ads8327_chip_deselect ();

    return (hi << 8) | lo;
}


void
ads8327_init (void)
{
    pio_config_set (SPI_ADC_CS_PIO, PIO_OUTPUT_HIGH);
    pio_config_set (SPI_ADC_CONV_PIO, PIO_OUTPUT_HIGH);

    /* Send wakeup command.  */
    ads8327_command (0xB000);
    
    /* Configure ADC.  Use internal clock.  */
    ads8327_command (0xE6FD);

    /* Select channel 0.  */
    ads8327_command (0x0000);
}


int16_t
ads8327_read (void)
{
    uint8_t adc_bytes[3];
    uint16_t val;

    ads8327_chip_select ();     

    adc_bytes[0] = spi_xferc (0xd0);
    adc_bytes[1] = spi_xferc (0x00);

    ads8327_chip_deselect ();

    val = (adc_bytes[1] << 8) | adc_bytes[0];

    return val;
}


int16_t
ads8327_convert (void)
{
    /* Drive CONV pin of A/D low to start conversion. 
       It only needs to go low for 40 ns.  */
    pio_output_low (SPI_ADC_CONV_PIO);
    /* Drive CONV pin of A/D high.  */
    pio_output_high (SPI_ADC_CONV_PIO);

    return ads8327_read ();
}


#include "config.h"
#include "pio.h"
#include "spi.h"


static inline void 
ads8325_chip_select (void)
{
    pio_output_low (SPI_ADC_CS_PIO);
}


static inline void 
ads8325_chip_deselect (void)
{
    pio_output_high (SPI_ADC_CS_PIO);
}


void
ads8325_init (void)
{
    pio_config_set (SPI_ADC_CS_PIO, PIO_OUTPUT_HIGH);
    pio_config_set (SPI_ADC_CONV_PIO, PIO_OUTPUT_HIGH);
}


int16_t
ads8325_read (void)
{
    uint8_t adc_bytes[3];
    uint16_t val;
    int8_t i;

    /* Drive CONV pin of A/D low to start conversion. 
       It only needs to go low for 40 ns.  */
    pio_output_low (SPI_ADC_CONV_PIO);
    /* Drive CONV pin of A/D high.  */
    pio_output_high (SPI_ADC_CONV_PIO);

    /* The ADS8325 is a successive approximation 16-bit ADC with an
       SPI interface.  It can operate at up to 100 kHz.  The serial
       clock determines the conversion speed.  A minimum of 22 clock
       cycles are required for 16-bit conversion.  The data should be
       sampled on the rising edge of the clock (SPI modes 0 or 3).
       For the first 5 rising clock edges the data line is high
       impedance.  It is then followed by a 0 bit and the 16 data bits
       (MSB first).
    */
    ads8325_chip_select ();     
    for (i = 0; i < 3; i++)
        adc_bytes[i] = spi_getc ();                             

    ads8325_chip_deselect ();

    /* Ignore the first 6 bits.  */
    val = (adc_bytes[0] & 3) << 14 | adc_bytes[1] << 6 | adc_bytes[2] >> 2;

    return val;
}


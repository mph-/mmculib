#include "config.h"
#include "pio.h"
#include "spi.h"
#include "spi_adc.h"


/* The ADS7870 is 12 bit while the ADS7871 is 14 bit.  */


enum
{
    ADS7870_CONVERT = 0x80,
    ADS7870_REG_READ = 0x40,
    ADS7870_REG_WRITE = 0x00,
    ADS7870_REG_16BIT = 0x20
};

/* Register addresses.  */
typedef enum
{
    ADS7870_RESULTLO,
    ADS7870_RESULTHI,
    ADS7870_PGAVALID,
    ADS7870_ADCTRL,      
    ADS7870_GAINMUX,      
    ADS7870_DIGIOSTATE,      
    ADS7870_DIGIOCTRL,      
    ADS7870_REFOSC
} ads7870_register_t;


/* Gain register settings.  */
typedef enum 
{
    ADS7870_PGA_GAIN_1 = 0, 
    ADS7870_PGA_GAIN_2 = 1, 
    ADS7870_PGA_GAIN_4 = 2, 
    ADS7870_PGA_GAIN_5 = 3, 
    ADS7870_PGA_GAIN_8 = 4,
    ADS7870_PGA_GAIN_10 = 5,
    ADS7870_PGA_GAIN_16 = 6,
    ADS7870_PGA_GAIN_20 = 7
} ads7870_pga_gain_t;


enum
{
    ADS7870_REFOSC_OSCR = BIT (5),
    ADS7870_REFOSC_OSCE = BIT (4),
    ADS7870_REFOSC_REFE = BIT (3),
    ADS7870_REFOSC_BUFE = BIT (2),
    ADS7870_REFOSC_R2V = BIT (1),
    ADS7870_REFOSC_RBE = BIT (0)
};


enum
{
    ADS7870_GAINMUX_CNVBSY = BIT (7)
};


static inline void 
ads7870_chip_select (void)
{
    pio_output_low (SPI_ADC_CS_PIO);
}


static inline void 
ads7870_chip_deselect (void)
{
    pio_output_high (SPI_ADC_CS_PIO);
}


#if 0
void
ads7870_debug (void)
{
    uint8_t byte0;
    uint8_t byte1;
    uint16_t val;
    char buffer[16];

    ads7870_chip_select ();     

    byte0 = spi_xferc (0xc0);
    byte1 = spi_xferc (0x00);

    ads7870_chip_deselect ();

    val = (byte0 << 8) | byte1;
    
    //sprintf (buffer, "%d\n", val);
    uint16toa (val, buffer, 0);
    usart0_puts (buffer);
}
#endif


static inline
void ads7870_reg_write (ads7870_register_t reg, uint8_t val)
{
    ads7870_chip_select ();

    spi_xferc (ADS7870_REG_WRITE | reg);
    spi_xferc (val);

    ads7870_chip_deselect ();
}


static inline
uint8_t ads7870_reg_read (ads7870_register_t reg)
{
    uint8_t val;

    ads7870_chip_select ();

    spi_xferc (ADS7870_REG_READ | reg);
    val = spi_xferc (0);

    ads7870_chip_deselect ();

    return val;
}


int16_t 
ads7870_read (void)
{
    uint16_t val;

    ads7870_chip_select ();

    spi_xferc (ADS7870_REG_READ | ADS7870_REG_16BIT | ADS7870_RESULTHI);
    val = spi_xferc (0) << 8;
    val |= spi_xferc (0);

    ads7870_chip_deselect ();

    return val;
}


static inline
int8_t ads7870_read_ready_p (void)
{
    return (ads7870_reg_read (ADS7870_GAINMUX) & ADS7870_GAINMUX_CNVBSY) != 0;
}


void
ads7870_channel_start (uint8_t channel, spi_adc_mode_t mode)
{
    switch (mode)
    {
    case SPI_ADC_MODE_SINGLE_ENDED:
        channel = (channel & 0x7) | 0x8;
        break;

    case SPI_ADC_MODE_DIFFERENTIAL:
        channel = channel & 0x3;
        break;

    case SPI_ADC_MODE_DIFFERENTIAL_INVERTED:
        channel = (channel & 0x3) | 0x4;
        break;
    }

    ads7870_chip_select ();

    spi_xferc (ADS7870_CONVERT | channel);
}


int16_t
ads7870_channel_convert (uint8_t channel, spi_adc_mode_t mode)
{
    ads7870_channel_start (channel, mode);

    /* Wait for conversion.  */
    while (! ads7870_read_ready_p ())
        continue;

    return ads7870_read ();
}


void
ads7870_init (void)
{
    pio_config_set (SPI_ADC_CS_PIO, PIO_OUTPUT_HIGH);

    /* Should check ID register.  */
    ads7870_reg_write (ADS7870_REFOSC, ADS7870_REFOSC_OSCE 
                       | ADS7870_REFOSC_REFE 
                       | ADS7870_REFOSC_BUFE);
}

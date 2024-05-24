#include "delay.h"
#include "pio.h"

#include "ledtape.h"

__attribute__((optimize (2)))
__always_inline__
static void ledtape_write_byte (pio_t pin, uint8_t byte)
{
    int j;

    for (j = 0; j < 8; j++)
    {
        pio_output_high (pin);
        DELAY_US (LEDTAPE_TPERIOD);
        // MSB first
        if (! (byte & 0x80))
            pio_output_low (pin);
        DELAY_US (LEDTAPE_TPERIOD);
        pio_output_low (pin);
        DELAY_US (LEDTAPE_TPERIOD);
        byte <<= 1;
    }
}


__attribute__((optimize (2)))
void ledtape_write (pio_t pin, uint8_t *buffer, uint16_t size)
{
    int i;

    // Send reset code
    pio_config_set (pin, PIO_OUTPUT_LOW);
    DELAY_US (100);

    // The data order is R G B per LED
    for (i = 0; i < size; i++)
    {
        ledtape_write_byte (pin, buffer[i]);
    }
}

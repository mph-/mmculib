#include "pio.h"
#include "delay.h"

void
panic (pio_t led_error_pio, unsigned int error_code)
{
    unsigned int i;

#ifdef LED_ACTIVE
    pio_output_set (led_error_pio, LED_ACTIVE);
#else
    pio_config_set (led_error_pio, PIO_OUTPUT_LOW);
#endif

    while (1)
    {
        for (i = 0; i < error_code; i++)
        {
            pio_output_toggle (led_error_pio);
            delay_ms (200);
            pio_output_toggle (led_error_pio);
            delay_ms (200);
        }
        delay_ms (1000);
    }
}

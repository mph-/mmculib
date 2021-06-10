#ifndef LEDTAPE_H
#define LEDTAPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "pio.h"

/**
 * @brief Write some WS2812B LED data out onto a pin.
 * @param pin The pin to write the data to.
 * @param buffer The raw GRB buffer (8 bits per colour, 24 bits per LED).
 * @param size The size of the buffer in bytes (LEDs * 3).
 */
void ledtape_write (pio_t pin, uint8_t *buffer, uint16_t size);

#ifdef __cplusplus
}
#endif
#endif


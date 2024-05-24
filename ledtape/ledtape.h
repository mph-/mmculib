#ifndef LEDTAPE_H
#define LEDTAPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "pio.h"

/*
 * TPERIOD may need to be manually tuned to generate the correct 800 kHz
 * waveforms. If LED tape is not working properly, check frequency of the
 * generated signal; if it is not close to 800 kHz, adjust the value of TPERIOD
 * accordingly. To *decrease* the frequency, *increase* the value.
 */
#ifndef LEDTAPE_TPERIOD
#define LEDTAPE_TPERIOD (0.4)  // value of 0.4 seems to work well
#endif

_Static_assert(LEDTAPE_TPERIOD > 0, "LEDTAPE_TPERIOD must be >0");

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


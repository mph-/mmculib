#ifndef LEDBUFFER_H
#define LEDBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "pio.h"

typedef struct ledbuffer
{
    pio_t pin;
    uint8_t* data;
    int leds;
} ledbuffer_t;

/**
  * @brief Create a new ledbuffer_t instance. Allocates all memory dynamically.
  * @param pin The PIO pin the LEDs are connected to.
  * @param leds The number of LEDS in the tape.
  * @return A pointer to the new ledbuffer_t instance.
  */
ledbuffer_t*
ledbuffer_init (pio_t pin, int leds);

/**
 * @brief Turn off all the LEDs in the tape.
 */
void
ledbuffer_clear (ledbuffer_t* buffer);

/**
 * @brief Set a specific LED in the tape with the given RGB values.
 * @param buffer the LED buffer to work with
 * @param index the position of the LED in the tape
 * @param r the red component (0-255)
 * @param g the green component (0-255)
 * @param b the blue component (0-255)
 */
void
ledbuffer_set (ledbuffer_t* buffer, uint8_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Advance the pattern along the tape by the given amount.
 *
 * Wraps the LEDs at the end of the tape back to the start.
 * @param buffer the LED buffer to work with
 * @param shift the amount to shift the pattern. Positive numbers move down the chain.
 */
void
ledbuffer_advance (ledbuffer_t* buffer, int shift);


/**
 * @brief Get the number of LEDs this buffer can hold.
 */
int
ledbuffer_size (ledbuffer_t* buffer);

/**
 * @brief Send the LED data out on the PIO pin. Should be called regularly.
 */
void
ledbuffer_write (ledbuffer_t* buffer);

#ifdef __cplusplus
}
#endif
#endif


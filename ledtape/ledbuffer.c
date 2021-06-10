
#include "ledbuffer.h"
#include "ledtape.h"

#include <stdlib.h>

ledbuffer_t*
ledbuffer_init(pio_t pin, int leds)
{
    ledbuffer_t* buffer;

    buffer = calloc(1, sizeof(*buffer));
    buffer->pin = pin;
    buffer->leds = leds;
    buffer->data = calloc(3, leds);

    return buffer;
}


void
ledbuffer_clear(ledbuffer_t* buffer)
{
    int i;
    for (i = 0; i < buffer->leds * 3; ++i) {
        buffer->data[i] = 0;
    }
}



void
ledbuffer_set(ledbuffer_t* buffer, uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    // ensure we are inside the buffer memory
    if (index >= buffer->leds)
        return;


    // Set the LED values (GRB ordering)
    buffer->data[index * 3 + 0] = g;
    buffer->data[index * 3 + 1] = r;
    buffer->data[index * 3 + 2] = b;
}



void
ledbuffer_advance(ledbuffer_t* buffer, int shift)
{
    int i, j;

    // make a copy of the existing LED buffer
    uint8_t* temp = malloc(buffer->leds * 3);
    for (i = 0; i < buffer->leds * 3; ++i)
        temp[i] = buffer->data[i];


    // now we can overwrite the old led data with the shift
    for (i = 0; i < buffer->leds; ++i) {
        if (i + shift < 0) {
            j = i + shift + buffer->leds;
        } else if (i + shift >= buffer->leds) {
            j = i + shift - buffer->leds;
        } else {
            j = i + shift;
        }

        buffer->data[j * 3 + 0] = temp[i * 3 + 0];
        buffer->data[j * 3 + 1] = temp[i * 3 + 1];
        buffer->data[j * 3 + 2] = temp[i * 3 + 2];
    }

    free(temp);
}



int
ledbuffer_size(ledbuffer_t* buffer)
{
    return buffer->leds * 3;
}



void
ledbuffer_write(ledbuffer_t* buffer)
{
    ledtape_write(buffer->pin, buffer->data, buffer->leds * 3);
}
/** @file   capture.h
    @author M. P. Hayes, UCECE
    @date   29 Dec 2019
*/

#ifndef CAPTURE_H
#define CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"


typedef uint16_t capture_sample_t;

typedef enum {CAPTURE_STOPPED, CAPTURE_STARTED, CAPTURE_TRIGGERED,
              CAPTURE_CAPTURED} capture_state_t;

typedef struct capture_struct capture_t;
    
typedef void (*capture_callback_t) (capture_t *);    

struct capture_struct
{
    capture_sample_t *buffer;
    capture_sample_t *dst;    
    capture_sample_t high_threshold;
    capture_sample_t low_threshold;
    uint16_t remaining;
    uint16_t pretrigger;
    uint16_t posttrigger;
    volatile capture_state_t state;
    uint8_t channels;
    // For debugging    
    int trigger_index;
    capture_sample_t trigger_value;
    uint16_t count;
    capture_callback_t callback;
};


capture_t *
capture_init (void);

void
capture_start (capture_t *cap,
               capture_sample_t *buffer,
               uint16_t pretrigger,
               uint16_t samples,
               uint8_t channels,
               capture_sample_t high_threshold,
               capture_sample_t low_threshold,
               capture_callback_t callback);
    
void
capture_stop (capture_t *cap);

void
capture_update (capture_t *cap,
                capture_sample_t *buffer,
                capture_sample_t *prev_buffer,                     
                uint16_t size);

bool
capture_ready_p (capture_t *cap);    
    

#ifdef __cplusplus
}
#endif    
#endif

    
    

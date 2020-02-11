/** @file   capture.c
    @author M. P. Hayes, UCECE
    @date   29 Dec 2019
*/

#include <stdlib.h>
#include <string.h>
#include "capture.h"


capture_t *
capture_init (void)
{
    capture_t *cap;

    cap = calloc (1, sizeof (*cap));
    cap->state = CAPTURE_STOPPED;

    return cap;
}


void
capture_start (capture_t *cap,
               capture_sample_t *buffer,
               uint16_t pretrigger,
               uint16_t samples,
               uint8_t channels,
               capture_sample_t high_threshold,
               capture_sample_t low_threshold,
               capture_callback_t callback)
{
    cap->buffer = buffer;
    cap->dst = buffer;    
    cap->pretrigger = pretrigger;
    cap->posttrigger = samples - pretrigger;    
    cap->remaining = samples;
    cap->channels = channels;
    cap->low_threshold = low_threshold;
    cap->high_threshold = high_threshold;        
    cap->trigger_index = -1;
    cap->trigger_value = 0;    
    cap->state = CAPTURE_STARTED;
    cap->callback = callback;
}


void
capture_stop (capture_t *cap)
{
    cap->state = CAPTURE_STOPPED;
}
              

void capture_done (capture_t *cap)
{
    cap->count++;
    
    if (cap->callback)
        cap->callback (cap);
}    


/** Scan buffer for trigger event.  This is either the sample value
    being below the low_threshold or above the high_threshold.  The
    sample number of the trigger event is returned or -1 if no trigger
    event is found.  */
static int capture_scan (capture_t *cap,
                         capture_sample_t *buffer,
                         uint16_t size,
                         capture_sample_t high_threshold,
                         capture_sample_t low_threshold)
{
    uint16_t i;

    for (i = 0; i < size; i++)
    {
        capture_sample_t val;

        val = buffer[i];
        if ((val >= high_threshold) || (val <= low_threshold))
            return i;
    }
    return -1;
}


/** This needs to be periodically called with the current and previous
    buffers.  The current buffer is scanned for a trigger event.  The
    previous buffer is required if the trigger event is within the
    required number of pretrigger samples.  Both buffer and prev_buffer
    consist of size samples.  */
void capture_update (capture_t *cap,
                     capture_sample_t *buffer,
                     capture_sample_t *prev_buffer,                     
                     uint16_t size)
{
    int trigger;
    int after_trigger;
    int count;    

    /* There are three capture cases:
       1.  The event fits within the current buffer.
       2.  The event fits within the previous and current buffer.
       3.  The event fits within the current and next buffer(s).

       
       Note, the number of pretrigger samples must fit within a buffer.
       
    */

    
    switch (cap->state)
    {
    case CAPTURE_STOPPED:
    case CAPTURE_CAPTURED:        
        break;

    case CAPTURE_STARTED:
        trigger = capture_scan (cap, buffer, size, cap->high_threshold,
                                cap->low_threshold);
        if (trigger == -1)
            break;

        cap->trigger_value = buffer[trigger];        
        // Truncate trigger to channel boundary.
        trigger = trigger >> (cap->channels - 1) << (cap->channels - 1);
        cap->trigger_index = trigger;
        
        after_trigger = size - trigger;
        
        if (trigger >= cap->pretrigger)
        {
            if (after_trigger >= cap->posttrigger)
            {
                /* Case 1:  The entire event fits in this buffer.  */
                memcpy (cap->dst, buffer + trigger - cap->pretrigger,
                        cap->remaining * sizeof (*cap->dst));
                cap->dst += cap->remaining;                
                cap->remaining = 0;
                cap->state = CAPTURE_CAPTURED;
                capture_done (cap);
            }
            else
            {
                /* Case 3a:  */
                count = size - trigger + cap->pretrigger;

                memcpy (cap->dst, buffer + trigger - cap->pretrigger,
                        count * sizeof (*cap->dst));
                cap->dst += count;
                cap->remaining -= count;
                // Wait for next buffer
                cap->state = CAPTURE_TRIGGERED;
            }
        }
        else
        {
            /* Case 2:  */            
            // Trigger occurred right at start.  Perhaps should add zeros?
            if (prev_buffer == 0)
                break;
            count = cap->pretrigger - trigger;
            memcpy (cap->dst, prev_buffer + size - count,
                    count * sizeof (*cap->dst));
            cap->dst += count;
            cap->remaining -= count;            

            count = size;
            if (count > cap->remaining)
                count = cap->remaining;
            
            memcpy (cap->dst, buffer, count * sizeof (*cap->dst));
            cap->dst += count;
            cap->remaining -= count;        
            
            if (cap->remaining == 0)
            {
                cap->state = CAPTURE_CAPTURED;
                capture_done (cap);
            }
            else
            {
                cap->state = CAPTURE_TRIGGERED;
            }
        }
        break;
        
    case CAPTURE_TRIGGERED:
        /* Case 3b:  */
        count = size;
        if (count > cap->remaining)
            count = cap->remaining;

        memcpy (cap->dst, buffer, count * sizeof (*cap->dst));
        cap->dst += count;
        cap->remaining -= count;        

        if (cap->remaining == 0)
        {
            cap->state = CAPTURE_CAPTURED;
            capture_done (cap);
        }
        break;
    }
}


bool capture_ready_p (capture_t *cap)
{
    return cap->state == CAPTURE_CAPTURED;
}

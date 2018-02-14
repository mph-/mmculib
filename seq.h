/** @file   seq.h
    @author M. P. Hayes, UCECE
    @date   1 April 2007
    @brief 
*/
#ifndef SEQ_H
#define SEQ_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

typedef struct
{
    /* Pointer to start of string.  */
    const char *str;
    /* Pointer to current position in string.  */    
    const char *cur;
    const char *(*callback) (void *data, const char *str);
    void *callback_data;
} seq_obj_t;


typedef seq_obj_t *seq_t;


/* Note the callback function can be called with a NULL pointer if
   seq_set has not been called before seq_update.  When the end of
   string is reached the sequencer does not automatically go back to
   the start of the string.  The callback function can call seq_get
   and return the initial string to repeat the sequence.

   The callback can return the address of a string (this must be
   statically allocated) and the sequencer will use the new string.
   However, when the end of string is reached, the sequencer will
   start with the old string.  */


/* Create a new sequencer with specified callback function and
   callback data.  */
extern seq_t seq_init (seq_obj_t *dev, 
                       const char * (*callback) (void *data, const char *str),
                       void *callback_data);


/* Set a new sequence.  */
static inline void
seq_set (seq_t seq, const char *str)
{
    seq->str = str;
    seq->cur = str;
}


/* Return start of current sequence.  */
static inline const char *
seq_get (seq_t seq)
{
    return seq->str;
}


/* Step to next element in sequence (note this depends on the callback
   function).  Returns non-zero at end of sequence.  */
extern int8_t seq_update (seq_t seq);


#ifdef __cplusplus
}
#endif    
#endif


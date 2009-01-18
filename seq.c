/** @file   seq.c
    @author M. P. Hayes, UCECE
    @date   1 April 2007
    @brief 
*/

#include <limits.h>
#include "seq.h"


seq_t
seq_init (seq_obj_t *seq,
          const char * (*callback) (void *data, const char *str),
          void *callback_data)
{
    seq->callback = callback;
    seq->callback_data = callback_data;
    seq->cur = 0;
    return seq;
}


int8_t
seq_update (seq_t seq)
{
    seq->cur = seq->callback (seq->callback_data, seq->cur);

    /* Return 1 if reached end of sequence.  */
    return *seq->cur == '\0';
}

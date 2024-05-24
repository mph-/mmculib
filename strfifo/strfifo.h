/** @file   strfifo.h
    @author M. P. Hayes
    @date   24 May 2024
    @brief  FIFO buffer for logging strings.  To prevent overflow,
            the oldest strings are discarded.
*/

#ifndef STRFIFO_H
#define STRFIFO_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct strfifo_struct
{
    int size;
    char *buffer;
    char *end;
    char *in;
    char *out;
} strfifo_t;


strfifo_t *strfifo_init (int size);

void strfifo_clear (strfifo_t *strfifo);

void strfifo_puts (strfifo_t *strfifo, char *str);

char *strfifo_gets (strfifo_t *strfifo);

#ifdef __cplusplus
}
#endif
#endif

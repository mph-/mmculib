#ifndef LINEBUFFER_H
#define LINEBUFFER_H

typedef struct linebuffer_struct linebuffer_t;


linebuffer_t *
linebuffer_init (int size);


void
linebuffer_add (linebuffer_t *linebuffer, char ch);


char
linebuffer_getc (linebuffer_t *linebuffer);


char *
linebuffer_gets (linebuffer_t *linebuffer, char *buffer, int size);

#endif

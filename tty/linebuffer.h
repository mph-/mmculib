/** @file   linebuffer.h
    @author M. P. Hayes
    @date   12 May 2015
    @brief  A wrapper for a ring buffer with primitive line editing
    support and mapping of carriage returns to newlines.
*/


#ifndef LINEBUFFER_H
#define LINEBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct linebuffer_struct linebuffer_t;

/** Initialise line buffer.
    @param size is the maximum linebuffer size
    @return pointer to linebuffer
*/
linebuffer_t *
linebuffer_init (int size);


/** Add new character to linebuffer.  Backspace will delete the
    previous character in the linebuffer (if present).  Carriage
    returns are converted to newlines.  */
void
linebuffer_add (linebuffer_t *linebuffer, char ch);


/** This is a non-blocking version of fgetc.
    @param linebuffer a pointer to the linebuffer
    @return next character from line buffer if it contains a newline
            otherwise -1.
*/
int
linebuffer_getc (linebuffer_t *linebuffer);


bool
linebuffer_full_p (linebuffer_t *linebuffer);

#ifdef __cplusplus
}
#endif
#endif

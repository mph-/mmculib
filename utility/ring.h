/** @file   ring.h
    @author M. P. Hayes, UCECE
    @date   15 May 2000
    @brief  Ring buffer interface.
*/

#ifndef _RING_H
#define _RING_H

#include "config.h"

typedef uint16_t ring_size_t;


/** Define ring buffer structure.  Unfortunately, since we need to
    statically allocate this structure we cannot make the structure
    opaque.  However, do not access the members directly.  They may
    change one day.  Note the in pointer is only modified by
    ring_write whereas the out pointer is only modified by ring_read
    so these routines can be called by an ISR without a race condition
    (provided pointer reads and writes are atomic).  */
typedef struct ring_struct
{
    char *in;                   /* Pointer to next element to write.  */
    char *out;                  /* Pointer to next element to read.  */
    char *top;                  /* Pointer to top of buffer.  */
    char *end;                  /* Pointer to char after buffer end.  */
} ring_t;


/** The following macros should be considered private.  */

/** Number of bytes in ring buffer.  */
#define RING_SIZE(RING) ((RING)->end - (RING)->top)

/** Number of bytes in ring buffer for reading.  Need temporary
   integer variable so that only read in and out pointers once in
   expression.  This avoids a possible race condition if these
   pointers are modified by an ISR.  It is assumed that pointer reads
   and writes are atomic.  */
#define RING_READ_NUM(RING, TMP) \
   (((TMP) = ((RING)->in - (RING)->out)) < 0 \
      ? (TMP) + RING_SIZE (RING) : (TMP))

/** Number of free bytes in ring buffer for writing.  */
#define RING_WRITE_NUM(RING, TMP) \
   (RING_SIZE (RING) - RING_READ_NUM (RING, TMP) - 1)


/** Return non-zero if the ring buffer is empty.  */
bool
ring_empty_p (ring_t *ring);


/** Return non-zero if the ring buffer is full.  */
bool
ring_full_p (ring_t *ring);


/** Read from a ring buffer.
    @param ring pointer to ring buffer structure
    @param buffer pointer to memory buffer
    @param size maximum number of bytes to read
    @return number of bytes actually read.  */
ring_size_t 
ring_read (ring_t *ring, void *buffer, ring_size_t size);


/** Write to a ring buffer.
    @param ring pointer to ring buffer structure
    @param buffer pointer to memory buffer
    @param size number of bytes to write
    @return number of bytes actually written.  */
ring_size_t
ring_write (ring_t *ring, const void *buffer, ring_size_t size);


/** Initialise a ring buffer structure to use a specified buffer.
    @param ring pointer to ring buffer structure
    @param buffer pointer to memory buffer
    @param size size of memory buffer in bytes
    @return size size of memory buffer in bytes or zero if error.  */
ring_size_t
ring_init (ring_t *ring, void *buffer, ring_size_t size);


/** Number of bytes in ring buffer for reading.  */
ring_size_t
ring_read_num (ring_t *ring);


/** Number of bytes free in ring buffer for writing.  */
ring_size_t
ring_write_num (ring_t *ring);


/** Determine where would write into ring buffer after size bytes.
    @param ring pointer to ring buffer structure
    @param size number of bytes to next write
    @return next write pointer.  */
char *
ring_write_next (ring_t *ring, ring_size_t size);


/** Determine where would read from ring buffer after size bytes.
    @param ring pointer to ring buffer structure
    @param size number of bytes to next read
    @return next read pointer.  */
char *
ring_read_next (ring_t *ring, ring_size_t size);


/** Advance ring buffer for writing.
    @param ring pointer to ring buffer structure
    @param size number of bytes to advance write
    @return new write pointer.  */
char *
ring_write_advance (ring_t *ring, ring_size_t size);


/** Advance ring buffer for reading.
    @param ring pointer to ring buffer structure
    @param size number of bytes to advance read
    @return new read pointer.  */
char *
ring_read_advance (ring_t *ring, ring_size_t size);


/** Write single character to ring buffer.
    @param ring pointer to ring buffer structure
    @param c character to write
    @return non-zero if successful.  */
static inline ring_size_t
ring_putc (ring_t *ring, char c)
{
    int tmp;
    char *ptr;

    /* Determine number of free entries in ring buffer
       and give up if full.  */
    if (!RING_WRITE_NUM (ring, tmp))
        return 0;

    ptr = ring->in;
    *ptr++ = c;
    if (ptr >= ring->end)
        ptr = ring->top;
    ring->in = ptr;

    return 1;
}


/** Read single character from ring buffer.
    @param ring pointer to ring buffer structure
    @return character or zero if unsuccessful.  */
static inline char
ring_getc (ring_t *ring)
{
    int tmp;
    char c;
    char *ptr;

    /* Determine number of free entries in ring buffer
       and give up if full.  */
    if (!RING_READ_NUM (ring, tmp))
        return 0;

    ptr = ring->out;
    c = *ptr++;
    if (ptr >= ring->end)
        ptr = ring->top;
    ring->out = ptr;

    return c;
}


/** Search for character in ring buffer. 
    @param ring pointer to ring buffer structure
    @param ch character to find
    @return non-zero if character found.  */
bool
ring_find (ring_t *ring, char ch);


/** Empties the ring buffer to it's original state.
    @param ring, pointer to ring buffer structure. */
void
ring_clear (ring_t *ring);


#endif

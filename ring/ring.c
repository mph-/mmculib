/** @file   ring.c
    @author M. P. Hayes, UCECE
    @date   15 May 2000
    @brief  Ring buffer implementation.
*/

#include <string.h>
#include <stdlib.h>
#include "ring.h"


/** Return non-zero if the ring buffer is empty.  */
bool
ring_empty_p (ring_t *ring)
{
    return ring_read_num (ring) == 0;
}


/** Return non-zero if the ring buffer is full.  */
bool
ring_full_p (ring_t *ring)
{
    return ring_write_num (ring) == 0;
}


/** Determine number of bytes in ring buffer ready for reading.
    @param ring pointer to ring buffer structure
    @return number of bytes in ring buffer ready for reading.  */
ring_size_t
ring_read_num (ring_t *ring)
{
    int tmp;

    return RING_READ_NUM (ring, tmp);
}


/** Determine number of bytes in ring buffer ready for reading without wrapping.
    @param ring pointer to ring buffer structure
    @return number of bytes in ring buffer ready for reading.  */
ring_size_t
ring_read_num_nowrap (ring_t *ring)
{
    int num;

    num = ring_read_num (ring);

    if (ring->out + num >= ring->end)
        return ring->end - ring->out;
    return num;
}


/** Determine number of bytes in ring buffer free for writing.
    @param ring pointer to ring buffer structure
    @return number of bytes in ring buffer free for writing.  */
ring_size_t
ring_write_num (ring_t *ring)
{
    int tmp;

    return RING_WRITE_NUM (ring, tmp);
}


/** Initialise a ring buffer structure to use a specified buffer.
    @param ring pointer to ring buffer structure
    @param buffer pointer to memory buffer
    @param size size of memory buffer in bytes
    @return size size of memory buffer in bytes or zero if error.  */
ring_t *
ring_init (ring_t *ring, void *buffer, ring_size_t size)
{
    if (! ring)
        ring = malloc (sizeof (*ring));
    if (! ring)
        return 0;

    if (! buffer)
        buffer = calloc (1, size);
    if (! buffer)
    {
        free (ring);
        return 0;
    }

    ring->top = buffer;
    ring->end = (char *)buffer + size;

    ring_clear (ring);

    return ring;
}


/** Read from a ring buffer.
    @param ring pointer to ring buffer structure
    @param buffer pointer to memory buffer
    @param size maximum number of bytes to read
    @return number of bytes actually read.  */
ring_size_t
ring_read (ring_t *ring, void *buffer, ring_size_t size)
{
    ring_size_t count;
    int tmp;
    char *buf = buffer;

    /* Determine number of entries in ring buffer.  */
    count = RING_READ_NUM (ring, tmp);
    if (size > count)
        size = count;

    /* Return if nothing to read.  */
    if (!size)
        return 0;

    if (ring->out + size >= ring->end)
    {
        int semi_num;

        /* The data is split into two portions, so first read the
           portion to the end of the ring buffer and transfer to the
           user's buffer.  */
        semi_num = ring->end - ring->out;
        memcpy (buf, ring->out, semi_num);

        /* Transfer data to user buffer from top of ring
           buffer.  */
        memcpy (buf + semi_num, ring->top, size - semi_num);

        /* Update output pointer.  */
        ring->out = ring->top + (size - semi_num);
    }
    else
    {
        /* Transfer data to user buffer.  */
        memcpy (buf, ring->out, size);

        /* Update output pointer.  */
        ring->out += size;
    }
    return size;
}


/** Write to a ring buffer.
    @param ring pointer to ring buffer structure
    @param buffer pointer to memory buffer
    @param size number of bytes to write
    @return number of bytes actually written.  */
ring_size_t
ring_write (ring_t *ring, const void *buffer, ring_size_t size)
{
    ring_size_t count;
    int tmp;
    const char *buf = buffer;

    /* Determine number of free entries in ring buffer.  */
    count = RING_WRITE_NUM (ring, tmp);

    /* Only write into ring buffer if can fit all the bytes.
       This is important if writing integers, structs, etc.  */
    if (count < size)
        return 0;

    if (ring->in + size >= ring->end)
    {
        int semi_num;

        /* The data is split into two portions, so first write the
           portion to the end of the ring buffer from the user's
           buffer.  */
        semi_num = ring->end - ring->in;
        memcpy (ring->in, buf, semi_num);

        /* Transfer data from user buffer to top of ring
           buffer.  */
        memcpy (ring->top, buf + semi_num, size - semi_num);

        /* Update input pointer.  */
        ring->in = ring->top + (size - semi_num);
    }
    else
    {
        /* Transfer data to user buffer.  */
        memcpy (ring->in, buf, size);

        /* Update input pointer.  */
        ring->in += size;
    }
    return size;
}


/** Write to a ring buffer.   Older data is overwritten to ensure
    the new data is written.
    @param ring pointer to ring buffer structure
    @param buffer pointer to memory buffer
    @param size number of bytes to write
    @return number of bytes actually written.  */
ring_size_t
ring_write_continuous (ring_t *ring, const void *buffer, ring_size_t size)
{
    ring_size_t write_num;
    ring_size_t ring_size;

    write_num = ring_write_num (ring);

    ring_size = RING_SIZE (ring);

    while (size > ring_size)
    {
        buffer += ring_size;
        size -= ring_size;
    }

    if (size > write_num)
    {
        /* Loose previous data.  */
        ring_read_advance (ring, size - write_num);
        size = write_num;
    }

    return ring_write (ring, buffer, size);
}


/** Determine where would write into ring buffer after size bytes.
    @param ring pointer to ring buffer structure
    @param size number of bytes to next write
    @return next write pointer.  */
char *
ring_write_next (ring_t *ring, ring_size_t size)
{
    char *new;

    new = ring->in + size;
    if (new >= ring->end)
        new -= RING_SIZE (ring);

    return new;
}


/** Determine where would read from ring buffer after size bytes.
    @param ring pointer to ring buffer structure
    @param size number of bytes to next read
    @return next read pointer.  */
char *
ring_read_next (ring_t *ring, ring_size_t size)
{
    char *new;

    new = ring->out + size;
    if (new >= ring->end)
        new -= RING_SIZE (ring);

    return new;
}


/** Advance ring buffer for writing.
    @param ring pointer to ring buffer structure
    @param size number of bytes to advance write
    @return new write pointer.  */
char *
ring_write_advance (ring_t *ring, ring_size_t size)
{
    return ring->in = ring_write_next (ring, size);
}


/** Advance ring buffer for reading.
    @param ring pointer to ring buffer structure
    @param size number of bytes to advance read
    @return new read pointer.  */
char *
ring_read_advance (ring_t *ring, ring_size_t size)
{
    return ring->out = ring_read_next (ring, size);
}


/** Search for character in ring buffer.
    @param ring pointer to ring buffer structure
    @param ch character to find
    @return non-zero if character found.  */
bool
ring_find (ring_t *ring, char ch)
{
    ring_size_t count;
    int tmp;
    char *ptr;

    /* Determine number of entries in ring buffer.  */
    count = RING_READ_NUM (ring, tmp);

    /* Return if nothing to read.  */
    if (!count)
        return 0;

    /* This could be optimised with two searches.  */
    ptr = ring->out;
    while (count--)
    {
        if (*ptr++ == ch)
            return 1;
        if (ptr >= ring->end)
            ptr = ring->top;
    }

    return 0;
}

/** Write single character to ring buffer.  If the buffer
    is full, overwrite last character.
    @param ring pointer to ring buffer structure
    @param c character to write
    @return non-zero if successful.  */
ring_size_t
ring_putc_force (ring_t *ring, char c)
{
    ring_size_t ret;

    ret = ring_putc (ring, c);
    if (ret != 0)
        return ret;

    if (ring->in == ring->top)
        ring->in = ring->end - 1;
    else
        ring->in--;

    return ring_putc (ring, c);
}


/** Empties the ring buffer to it's original state.
    @param ring, pointer to ring buffer structure. */
void
ring_clear (ring_t *ring)
{
    ring->in = ring->out = ring->top;
}

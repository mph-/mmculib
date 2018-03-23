#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../ring.h"

//#define N 1000000
#define N 100000

#define M 80

int main (void)
{
    char *buffer1;
    char *buffer2;
    char *rbuffer;
    int i;
    int j;

    srand (7);

    /* Valgrind does not work well with stack allocated arrays.  */
    
    buffer1 = malloc (N);
    buffer2 = malloc (N);
    rbuffer = malloc (M);
    
    for (i = 0; i < N; i++)
        buffer1[i] = i % 100;

    
    for (j = 0; j < 1000; j++)
    {
        ring_t ring;
        int in = 0;
        int out = 0;
        int wsize;
        int rsize;
        int wbytes;
        int rbytes;        

        ring_init (&ring, rbuffer, M);
        
        while ((in + M) < N)
        {
            ring_t rring;
            ring_t wring;            
            
            wsize = rand () & 0x63;
            rsize = rand () & 0x63;
            //rsize = 1;

            memcpy (&wring, &ring, sizeof (ring));
            
            wbytes = ring_write (&ring, &buffer1[in], wsize);

            memcpy (&rring, &ring, sizeof (ring));
            
            rbytes = ring_read (&ring, &buffer2[out], rsize);

            printf ("In %d, Out %d, Wrote %d->%d, Count %d, Read %d->%d, Count %d\n",
                    in, out, wsize, wbytes, ring_read_num (&rring),
                    rsize, rbytes, ring_read_num (&ring));

            for (i = 0; i < rbytes; i++)
            {
                if (buffer1[out + i] != buffer2[out + i])
                {
                    printf ("Mismatch at %d read %d, expected %d for trial %d\n",
                            i, buffer2[out + i], buffer1[out + i], j);
                    return 1;
                }
            }
            
            in += wbytes;
            out += rbytes;

            if (out > in)
            {
                printf ("out %d, in %d\n", out, in);
                return 1;
            }
        }
    }

    free (buffer1);
    free (buffer2);
    
    return 0;
}

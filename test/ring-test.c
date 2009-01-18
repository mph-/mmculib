#include <stdio.h>
#include <string.h>
#include "ring.h"


int main (void)
{
    ring_t ring;                /* Ring buffer structure.    */
    char buffer[128];           /* Ring buffer data.    */
    char *msg1 = "abcde";
    char *msg2 = "fghij";
    char foo[64];
    int num;
    
    ring_init (&ring, buffer, sizeof (buffer));

    ring_write (&ring, msg1, strlen (msg1));

    ring_write (&ring, msg2, strlen (msg2));

    num = ring_read (&ring, foo, sizeof (foo));
    foo[num] = '\0';

    printf ("%d: %s\n", num, foo);

    return 0;
}

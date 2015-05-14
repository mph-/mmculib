#include <stdio.h>
#include "../bits.h"

int main(void)
{
    unsigned int status = 0;

    if (BITS_MASK (0, 3) != 15)
        printf ("BITS_MASK error\n");
    if (BITS_MASK (1, 4) != 30)
        printf ("BITS_MASK error\n");

    BITS_INSERT (status, 7, 0, 3);
    if (status != 7)
        printf ("BITS_INSERT error\n");
    
    status = 0;
    BITS_INSERT (status, 7, 1, 4);
    if (status != 14)
        printf ("BITS_INSERT error\n");

    status = 0;
    BITS_INSERT (status, 1, 31, 31);
    if (status != (1 << 31))
        printf ("BITS_INSERT error\n");

    return 0;
}

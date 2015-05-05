#include <stdio.h>
#include "../utility/uint8toa.h"

int main (void)
{
    char str[16];
    char *p;

    uint8toa (0, str, 0);
    printf ("%s\n", str);

    uint8toa (9, str, 0);
    printf ("%s\n", str);

    uint8toa (99, str, 0);
    printf ("%s\n", str);

    uint8toa (199, str, 0);
    printf ("%s\n", str);

    uint8toa (9, str, 1);
    printf ("%s\n", str);

    uint8toa (99, str, 1);
    printf ("%s\n", str);

    uint8toa (199, str, 1);
    printf ("%s\n", str);

    p = str;
    while (*p++)
        continue;

    uint8toa (199, p - 1, 1);
    printf ("%s\n", str);

    return 0;
}

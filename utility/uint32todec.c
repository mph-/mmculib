#include "config.h"

/* Convert NUMBER to decimal using DIGITS digits and
   store in buffer STR.  Assume that STR can hold the 
   desired number of digits.  */
void uint32todec (uint32_t number, char *str, unsigned int digits, 
                  bool leading_zeros)
{
    unsigned int i;
    
    for (i = 0; i < digits; i++)
    {
	int val;

        if (!leading_zeros && !number && i)
            break;

	/* Determine lower 4 bits.  */
	val = number % 10;
	
        str[digits - i - 1] = '0' + val;

	number = number / 10;
    }

    /* Null terminate string.  */
    str[i] = '\0';
}

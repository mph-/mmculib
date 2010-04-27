#include "config.h"

/* Convert NUMBER to hexadecimal using DIGITS digits and
   store in buffer STR.  Assume that STR can hold the 
   desired number of digits.  */
void uint32tohex (uint32_t number, char *str, unsigned int digits, 
                  bool leading_zeros)
{
    unsigned int i;
    
    for (i = 0; i < digits; i++)
    {
	int val;

        if (!leading_zeros && !number && i)
            break;

	/* Determine lower 4 bits.  */
	val = number & 0x0f;;
	
	if (val < 10)
	    str[digits - i - 1] = '0' + val;
	else
	    str[digits - i - 1] = 'A' + val - 10;

	number = number >> 4;
    }

    /* Null terminate string.  */
    str[i] = '\0';
}

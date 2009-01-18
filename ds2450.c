/** @file   ds2450.c
    @author M. P. Hayes, UCECE
    @date   08 June 2002
    @brief 
*/
#include <stdio.h>
#include "ds2450.h"
#include "dscrc16.h"
#include "u1wire.h"
#include "delay.h"


enum { DS2450_FAMILY_CODE = 0x20 };

enum { DS2450_MEMORY_BYTES = 24 };

enum 
{ 
    DS2450_CONVERT = 0x3c,
    DS2450_READ_MEMORY = 0xaa,
    DS2450_WRITE_MEMORY = 0x55
};


#ifndef DS2450_DEBUG
#define DS2450_DEBUG 1
#endif
#ifndef DS2450_CRC_CHECK
#define DS2450_CRC_CHECK 1
#endif


/* The DS2450 has 4 ADC channels.  These can be converted separately.
   I guess we can have two modes:
   mode 1 where all channels get converted then we pick the desired 
      channels,
   mode 2 where we specify the channel (or perhaps channels) that we
      want to convert then we get given the conversion value(s).
   
   Conceptually, I think mode 2 is simpler (especially if we limit the
   options to a single channel).
*/

/* A conversion takes between 60--80 microseconds per bit plus 160
   microseconds.  Thus the maximum time to convert 1 channel is 160 +
   12 * 80 = 1.12 ms and the time to convert 4 channels is 4 * 12 * 80
   = 4 ms.  */
int8_t
ds2450_adc_conversion_start (u1wire_t dev, uint8_t channel_mask)
{
    uint8_t readout_control;
    crc16_t crc;
    crc16_t crc2;

    /* Preset output registers to zero.  */
    readout_control = 0x55;

    u1wire_command (dev, DS2450_CONVERT);
    u1wire_write (dev, &channel_mask, sizeof (channel_mask));
    u1wire_write (dev, &readout_control, sizeof (readout_control));

    u1wire_read (dev, &crc, sizeof (crc));

#if DS2450_CRC_CHECK
    crc2 = dscrc16_byte (0, DS2450_CONVERT);
    crc2 = dscrc16_byte (crc2, channel_mask);
    crc2 = dscrc16_byte (crc2, readout_control);
    if (crc != ~crc2)
        printf ("! ds2450 crc convert error, crc = %u, %u\n", crc, ~crc2);
#endif
    
    return 1;
}


bool
ds2450_adc_ready_p (u1wire_t dev)
{
    return u1wire_ready_p ();
}


static int8_t
ds2450_memory_read (u1wire_t dev, uint16_t addr, uint8_t *data, uint8_t size)
{
    crc16_t crc;
    crc16_t crc2;

    u1wire_command (dev, DS2450_READ_MEMORY);

    u1wire_write (dev, &addr, sizeof (addr));

    u1wire_read (dev, data, size);
    u1wire_read (dev, &crc, sizeof (crc));

#if DS2450_CRC_CHECK
    crc2 = dscrc16_byte (0, DS2450_READ_MEMORY);
    crc2 = dscrc16 (crc2, &addr, sizeof (addr));
    crc2 = dscrc16 (crc2, data, size);

    if (crc != ~crc2)
        printf ("! ds2450 crc read error, crc = %u, %u\n", crc, ~crc2);
#endif
    return size;
}


static int8_t
ds2450_memory_write (u1wire_t dev, uint16_t addr, uint8_t *data, uint8_t size)
{
    crc16_t crc;
    crc16_t crc2;
    int8_t i;

    u1wire_command (dev, DS2450_WRITE_MEMORY);

    u1wire_write (dev, &addr, sizeof (addr));

    for (i = 0; i < size; i++)
    {
        crc16_t crc;
        uint8_t val;

        /* Send byte.  */
        u1wire_byte_write (data[i]);

        /* Read crc.  */
        u1wire_read (dev, &crc, sizeof (crc));
        
        /* Read value written to memory, this should match written value.  */
        u1wire_read (dev, &val, sizeof (val));
    }

    return i;
}
    

int8_t
ds2450_adc_read (u1wire_t dev, uint8_t channel_mask, uint16_t *adc)
{
    int i, j;
    uint8_t data[8];

    /* Read channels A, B, C, D, least significant byte first.  */
    ds2450_memory_read (dev, 0, data, sizeof (data));

    for (i = j = 0; channel_mask; channel_mask >>= 1, j++)
    {
        if (channel_mask & 1)
            adc[i++] = data[j * 2 + 1] * 256 + data[j * 2];
    }

    return i;
}


bool
ds2450_device_p (u1wire_obj_t *dev)
{
    return dev->rom_code.fields.family == DS2450_FAMILY_CODE;
}


void
ds2450_debug (u1wire_t dev)
{
#if DS2450_DEBUG
    uint8_t data[8];

    ds2450_memory_read (dev, 0, data, sizeof (data));
    printf ("%02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
            data[0], data[1], data[2], data[3], 
            data[4], data[5], data[6], data[7]);

    ds2450_memory_read (dev, 8, data, sizeof (data));
    printf ("%02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
            data[0], data[1], data[2], data[3], 
            data[4], data[5], data[6], data[7]);

    ds2450_memory_read (dev, 16, data, sizeof (data));
    printf ("%02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
            data[0], data[1], data[2], data[3], 
            data[4], data[5], data[6], data[7]);
#endif
}


u1wire_t
ds2450_init (u1wire_obj_t *dev)
{
    uint8_t i;
    uint8_t data[8];

    if (! ds2450_device_p (dev))
        return NULL;

    for (i = 0; i < 4; i++)
    {
        /* Convert 16 bits and make pin input.  */
        data[i * 2] = 0;
        /* Reset POR and set input range to 5.1 V.  */
        data[i * 2 + 1] = 1;
    }

    ds2450_memory_write (dev, 8, data, 8);

    /* For VCC powered devices we need to write 0x40 to address
       0x1c.  */
    data[0] = 0x40;
    ds2450_memory_write (dev, 0x1c, data, 1);

    return dev;
}

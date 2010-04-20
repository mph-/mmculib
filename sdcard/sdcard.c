#include "sdcard.h"

/*  The SD Card wakes up in the SD Bus mode.  It will enter SPI mode
    if the CS signal is asserted during the reception of the reset
    command (CMD0).

    The default command structure/protocol for SPI mode is that CRC
    checking is disabled.  Since the card powers up in SD Bus mode,
    CMD0 must be followed by a valid CRC byte (even though the command
    is sent using the SPI structure).  Once in SPI mode, CRCs are
    disabled by default.

    The host starts every bus transaction by asserting the CS signal
    low.  */

enum {SD_CMD_LEN = 6};

typedef enum 
{
    SD_OP_RESET = 0,              /* CMD0 */
    SD_OP_SEND_OP_COND = 1,       /* CMD1 */
    SD_OP_SEND_CSD = 9,           /* CMD9 */
    SD_OP_SET_BLOCKLEN = 16,           /* CMD16 */
    SD_OP_READ_SINGLE_SECTOR = 17, /* CMD17 */
    SD_OP_WRITE_SECTOR = 24,       /* CMD24 */
    SD_OP_WRITE_MULTIPLE_SECTOR = 25, /* CMD25 */
    SD_OP_READ_OCR = 58,              /* CMD58 */
    SD_OP_CRC_ON_OFF = 59,            /* CMD59 */
} sdcard_op_t;


/* The command format is 6 bytes:
   start bit (0)
   host bit (1)
   command 6-bits
   argument 32-bits
   CRC 7-bits
   stop bit (1)
*/

#define SD_HOST_BIT (BIT (6))
#define SD_STOP_BIT (BIT (0))


#ifndef SDCARD_DEVICES_NUM
#define SDCARD_DEVICES_NUM 4
#endif 

#define SDCARD_RETRIES_NUM 256


static uint8_t sdcard_devices_num = 0;
static sdcard_dev_t sdcard_devices[SDCARD_DEVICES_NUM];


/* The 16-bit CRC uses a standard CCIT generator polynomial:
   x^16 + x^12 + x^5 + 1   */


static uint16_t 
sdcard_crc16_bit (uint16_t crc, uint8_t in)
{
    uint8_t bit0;

    /* NB, the CRC is stored in reverse order to that specified
       by the polynomial.  */
    bit0 = crc & 1;

    crc >>= 1;    
    if (bit0 ^ in)
        crc = crc ^ (BIT (15 - 0) | BIT (15 - 5) | BIT (15 - 12));

    return crc;
}


uint16_t
sdcard_crc16_byte (uint16_t crc, uint8_t val)
{
    uint8_t i;
    
    for (i = 0; i < 8; i++)
    {
        crc = sdcard_crc16_bit (crc, val & 1);
        val >>= 1;
    }
    return crc;
}


uint16_t
sdcard_crc16 (uint16_t crc, const void *bytes, uint16_t size)
{
    uint8_t i;
    const uint8_t *data = bytes;
    
    for (i = 0; i < size; i++)
        crc = sdcard_crc16_byte (crc, data[i]);
    
    return crc;
}


/* The 7-bit CRC uses a generator polynomial:
   x^7 + x^3 + 1   */

static uint8_t 
sdcard_crc7_bit (uint8_t crc, uint8_t in)
{
    uint8_t bit0;
    
    /* NB, the CRC is stored in reverse order to that specified
       by the polynomial.  */
    bit0 = crc & 1;

    crc >>= 1;    
    if (bit0 ^ in)
        crc = crc ^ (BIT (7 - 0) | BIT (7 - 3));
                     
    return crc;
}


uint8_t
sdcard_crc7_byte (uint8_t crc, uint8_t val)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        crc = sdcard_crc7_bit (crc, val & 1);
        val >>= 1;
    }
    return crc;
}


uint8_t
sdcard_crc7 (uint8_t crc, const void *bytes, uint8_t size)
{
    uint8_t i;
    const uint8_t *data = bytes;

    for (i = 0; i < size; i++)
        crc = sdcard_crc7_byte (crc, data[i]);

    return crc;
}


// Keeps clocking the SD card until the desired byte is returned from the card
bool
sdcard_response_match (sdcard_t dev, uint8_t desired)
{
    uint16_t retries;
    uint8_t response;
    
    // Keep reading the SD card for the desired response
    for (retries = 0; retries < SDCARD_RETRIES_NUM; retries++)
    {
        spi_read (dev->spi, &response, 1, 0);
        if (response == desired)
            return 1;
    }
    return 0;
}


static void
sdcard_deselect (sdcard_t dev)
{
    uint8_t dummy[1] = {0xff};

    /* After the last SPI bus transaction, the host is required to
       provide 8 clock cycles for the card to complete the operation
       before shutting down the clock. Throughout this 8-clock period,
       the state of the CS signal is irrelevant.  It can be asserted
       or de-asserted.  */

    spi_write (dev->spi, dummy, sizeof (dummy), 1);
}


static uint8_t
sdcard_command (sdcard_t dev, sdcard_op_t op, uint32_t param)
{
    uint8_t command[SD_CMD_LEN];
    uint8_t response[SD_CMD_LEN];
    uint16_t retries;
    
    command[0] = op | SD_HOST_BIT;
    command[1] = param >> 24;
    command[2] = param >> 16;
    command[3] = param >> 8;
    command[4] = param;
    command[5] = (sdcard_crc7 (0, command, 5) << 1) | SD_STOP_BIT;
        
    /* Send command and read R1 response.  */
    spi_transfer (dev->spi, command, response, SD_CMD_LEN, 0);

    for (retries = 0; retries < SDCARD_RETRIES_NUM; retries++)
    {
        spi_read (dev->spi, response, 1, 0);
        if (! (response[0] & 0x80))
            break;
    }

    return response[0];
}


// Write a 512 byte sector at a given location (according to Sandisk SD
// card product manual, 512 bytes is minimum sector len see pp5-1)
uint16_t
sdcard_write_sector (sdcard_t dev, sdcard_addr_t addr, const void *buffer,
                     sdcard_sector_t sector)
{
    uint8_t status;
    uint16_t crc;
    uint8_t command[2];
    uint8_t response[1];

    addr = sector * SDCARD_SECTOR_SIZE;

    status = sdcard_command (dev, SD_OP_WRITE_SECTOR, addr);
    if (status != 0)
    {
        sdcard_deselect (dev);
        return 0;
    }

    crc = sdcard_crc16 (0, buffer, SDCARD_SECTOR_SIZE);
    
    /* Send data begin token.  */
    command[0] = 0xFE;
    spi_write (dev->spi, command, 1, 1);

    /* Send the data.  */
    spi_write (dev->spi, buffer, SDCARD_SECTOR_SIZE, 1);

    command[0] = crc >> 8;
    command[1] = crc & 0xff;

    /* Send the crc.  */
    spi_write (dev->spi, command, 2, 1);

    /* Get the status response.  */
    command[0] = 0xff;
    spi_transfer (dev->spi, command, response, 1, 1);    
    
    /* Check to see if the data was accepted.  */
    if ((response[0] & 0x0F) != 0x05)
    {
        sdcard_deselect (dev);
        return 0;
    }
    
    /* Wait for card to complete write cycle.  */
    if (!sdcard_response_match (dev, 0x00))
    {
        sdcard_deselect (dev);
        return 0;
    }
    
    sdcard_deselect (dev);

    return SDCARD_SECTOR_SIZE;
}


// Read a 512 sector of data on the SD card
sdcard_ret_t 
sdcard_read_sector (sdcard_t dev, sdcard_addr_t addr, void *buffer,
                    sdcard_sector_t sector)
{
    return 0;
}


sdcard_ret_t
sdcard_read (sdcard_t dev, sdcard_addr_t addr, void *buffer, sdcard_size_t size)
{
    uint16_t sectors;
    uint16_t i;
    sdcard_size_t total;
    sdcard_size_t bytes;
    uint8_t *dst;

    /* Ignore partial reads.  */
    sectors = size / SDCARD_SECTOR_SIZE;
    dst = buffer;
    total = 0;
    for (i = 0; i < sectors; i++)
    {
        bytes = sdcard_read_sector (dev, addr + i, dst, size);
        if (!bytes)
            return total;
        dst += bytes;
        total += bytes;
    }
    return total;
}


sdcard_ret_t
sdcard_write (sdcard_t dev, sdcard_addr_t addr, const void *buffer,
              sdcard_size_t size)
{
    uint16_t sectors;
    uint16_t i;
    sdcard_size_t total;
    sdcard_size_t bytes;
    const uint8_t *src;

    /* Ignore partial writes.  */
    sectors = size / SDCARD_SECTOR_SIZE;
    src = buffer;
    total = 0;
    for (i = 0; i < sectors; i++)
    {
        bytes = sdcard_write_sector (dev, addr + i, src, size);
        if (!bytes)
            return total;
        src += bytes;
        total += bytes;
    }
    return total;
}


sdcard_err_t
sdcard_probe (sdcard_t dev)
{
    uint8_t status;
    int retries;
    uint8_t dummy[10] = {0xff, 0xff, 0xff, 0xff, 0xff,
                         0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t command[4];
    uint8_t response[4];

    /* Send the card 80 clocks to activate it.  */
    spi_write (dev->spi, dummy, sizeof (dummy), 0);

    status = sdcard_command (dev, SD_OP_RESET, 0);
    if (status != 0x01)
        return SDCARD_ERR_NO_CARD;

    sdcard_deselect (dev);

    /* The card should be in the idle state.  The only valid commands
       are READ_OCR (CMD58) and CMD59.  */

    /* Read operation condition register.  */
    for (retries = 0; retries < 65536; retries++)
    {
        status = sdcard_command (dev, SD_OP_READ_OCR, 0);
        if (status <= 1)
            break;
    }
    if (status > 1)
    {
        sdcard_deselect (dev);
        return SDCARD_ERR_ERROR;
    }

    spi_transfer (dev->spi, command, response, 4, 0);    
    
    /* Should check to see if card happy with our supply voltage.  */

    for (retries = 0; retries < 65536; retries++)
    {
        status = sdcard_command (dev, SD_OP_SEND_OP_COND, BIT (30));
        if (status == 0)
            break;
    }
    if (status != 0)
    {
        sdcard_deselect (dev);
        return SDCARD_ERR_ERROR;
    }

    status = sdcard_command (dev, SD_OP_SET_BLOCKLEN, SDCARD_SECTOR_SIZE);
    if (status != 0)
    {
        sdcard_deselect (dev);
        return SDCARD_ERR_ERROR;
    }

    return SDCARD_ERR_OK;
}


sdcard_t
sdcard_init (const sdcard_cfg_t *cfg)
{
    sdcard_t dev;

    if (sdcard_devices_num >= SDCARD_DEVICES_NUM)
        return 0;
    dev = sdcard_devices + sdcard_devices_num;
 
    dev->spi = spi_init (&cfg->spi);
    if (!dev->spi)
        return 0;

    // Hmmm, should we let the user override the mode?
    spi_mode_set (dev->spi, SPI_MODE_0);
    spi_cs_mode_set (dev->spi, SPI_CS_MODE_FRAME);
   
    return dev;
}


void
sdcard_shutdown (sdcard_t dev)
{
    // TODO
    spi_shutdown (dev->spi);
}

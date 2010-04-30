/* Secure digital card driver.  */
#include <string.h>
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
    low.  

    SanDisk cards allow partial reads (down to 1 byte) but not partial
    writes.  Writes have a minimum block size of 512 bytes. 

    In SPI mode, CRC checks are disabled by default.

    The maximum SPI clock speed is 25 MHz.
 */

enum {SD_CMD_LEN = 6};

typedef enum 
{
    SD_OP_GO_IDLE_STATE = 0,          /* CMD0 */
    SD_OP_SEND_OP_COND = 1,           /* CMD1 */
    SD_OP_SEND_CSD = 9,               /* CMD9 */
    SD_OP_SET_BLOCKLEN = 16,          /* CMD16 */
    SD_OP_READ_BLOCK = 17,            /* CMD17 */
    SD_OP_WRITE_BLOCK = 24,           /* CMD24 */
    SD_OP_WRITE_MULTIPLE_BLOCK = 25,  /* CMD25 */
    SD_OP_READ_OCR = 58,              /* CMD58 */
    SD_OP_CRC_ON_OFF = 59,            /* CMD59 */
} sdcard_op_t;


enum 
{
    SD_WRITE_OK = 5,
    SD_WRITE_CRC_ERROR = 11,
    SD_WRITE_ERROR = 13
};


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

uint8_t
sdcard_crc7_byte (uint8_t crc, uint8_t val, uint8_t bits)
{
    int i;
    
    for (i = bits; i--; val <<= 1)
    {
        crc = (crc << 1) | ((val & 0x80) ? 1 : 0);
        
        if (crc & 0x80)
            crc ^= (BIT(0) | BIT(3));
    }
    return crc & 0x7f;
}


uint8_t
sdcard_crc7 (uint8_t crc, const void *bytes, uint8_t size)
{
    uint8_t i;
    const uint8_t *data = bytes;

    for (i = 0; i < size; i++)
        crc = sdcard_crc7_byte (crc, data[i], 8);

    crc = sdcard_crc7_byte (crc, 0, 7);

    return crc;
}


/*  Keeps polling the SD card until the desired byte is returned.  */
bool
sdcard_response_match (sdcard_t dev, uint8_t desired)
{
    uint16_t retries;
    uint8_t response[1];
    
    // Keep reading the SD card for the desired response
    for (retries = 0; retries < SDCARD_RETRIES_NUM; retries++)
    {
        response[0] = 0xff;
        spi_transfer (dev->spi, response, response, 1, 0);
        if (response[0] == desired)
            return 1;
    }
    return 0;
}


static void
sdcard_deselect (sdcard_t dev)
{
    uint8_t dummy[1] = {0xff};

    spi_cs_disable (dev->spi);

    /* After the last SPI bus transaction, the host is required to
       provide 8 clock cycles for the card to complete the operation
       before shutting down the clock. Throughout this 8-clock period,
       the state of the CS signal is irrelevant.  It can be asserted
       or de-asserted.  */

    spi_write (dev->spi, dummy, sizeof (dummy), 1);

    spi_cs_enable (dev->spi);
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
        
    /* Send command; the card will respond with a sequence of 0xff.  */
    spi_transfer (dev->spi, command, response, SD_CMD_LEN, 0);

    command[0] = 0xff;

    /* Search for R1 response; the card should respond with 0 to 8
       bytes of 0xff beforehand.  */
    for (retries = 0; retries < 65; retries++)
    {
        spi_transfer (dev->spi, command, &dev->status, 1, 0);
        
#if 0
        /* This doesn't seem to work.  Sometimes I get a response of 0x7f
           which trips this test.  */
        if (! (dev->status & 0x80))
            break;

#else
        /* Check for R1 response.  */
        if ((op == SD_OP_GO_IDLE_STATE && dev->status == 0x01)
            || (op != SD_OP_GO_IDLE_STATE && dev->status == 0x00))
            break;
#endif
    }

    return dev->status;
}


bool
sdcard_csd_read (sdcard_t dev, uint8_t *csd, uint8_t bytes)
{
    uint8_t crc[2];
    uint8_t status;

    status = sdcard_command (dev, SD_OP_SEND_CSD, 0);
    if (status != 0)
    {
        sdcard_deselect (dev);
        return 0;
    }

    /* Wait for card to return the start data token.  */
    if (!sdcard_response_match (dev, 0xfe))
    {
        sdcard_deselect (dev);
        return 0;
    }

    /* Read the 128 bits of data.  */
    memset (csd, 0xff, bytes);
    spi_transfer (dev->spi, csd, csd, bytes, 0);

    /* Read the 16 bit crc.  */
    memset (crc, 0xff, sizeof (crc));
    spi_transfer (dev->spi, crc, crc, sizeof (crc), 0);

    sdcard_deselect (dev);
    
    return 1;
}


sdcard_addr_t
sdcard_capacity (sdcard_t dev)
{
    uint8_t csd[16];
    uint16_t c_size;
    uint16_t c_size_mult;
    uint16_t read_bl_len;
    uint16_t block_len;
    sdcard_addr_t capacity;

    if (!sdcard_csd_read (dev, csd, sizeof (csd)))
        return 0;

    /* C_SIZE bits 70:62
       C_SIZE_MULT bits 49:47
       READ_BL_LEN bits 83:80
    */

    c_size = ((csd[7] & 0x7f) << 2) | (csd[8] >> 6);
    c_size_mult = ((csd[9] & 0x03) << 1) | (csd[10] >> 7);
    read_bl_len = csd[5] & 0x0f;
    
    block_len = 1 << read_bl_len;
    capacity = c_size * (1LL << (c_size_mult + 2)) * block_len;

    return capacity;
}


bool
sdcard_page_erase (sdcard_t dev, sdcard_addr_t addr)
{

    return 1;
}


// Read a 512 block of data on the SD card
sdcard_ret_t 
sdcard_block_read (sdcard_t dev, sdcard_addr_t addr, void *buffer)
{
    uint8_t status;
    uint8_t command[2];

    status = sdcard_command (dev, SD_OP_READ_BLOCK, addr);
    if (status != 0)
    {
        sdcard_deselect (dev);
        return 0;
    }

    /* Send data begin token.  */
    command[0] = 0xFE;
    spi_write (dev->spi, command, 1, 1);

    /* Read the data.  */
    spi_read (dev->spi, buffer, SDCARD_BLOCK_SIZE, 0);

    /* Read the crc.  */
    spi_read (dev->spi, command, 2, 0);

    sdcard_deselect (dev);

    return SDCARD_BLOCK_SIZE;
}


sdcard_ret_t
sdcard_read (sdcard_t dev, sdcard_addr_t addr, void *buffer, sdcard_size_t size)
{
    uint16_t blocks;
    uint16_t i;
    sdcard_size_t total;
    sdcard_size_t bytes;
    uint8_t *dst;

    /* Ignore partial reads.  */
    if (addr % SDCARD_BLOCK_SIZE || size % SDCARD_BLOCK_SIZE)
        return 0;

    blocks = size / SDCARD_BLOCK_SIZE;
    dst = buffer;
    total = 0;
    for (i = 0; i < blocks; i++)
    {
        bytes = sdcard_block_read (dev, addr + i, dst);
        if (!bytes)
            return total;
        dst += bytes;
        total += bytes;
    }
    return total;
}


uint16_t
sdcard_erased_block_write (sdcard_t dev, sdcard_addr_t addr, const void *buffer)
{
    uint8_t status;
    uint16_t crc;
    uint8_t command[2];
    uint8_t response[1];

    status = sdcard_command (dev, SD_OP_WRITE_BLOCK, addr);
    if (status != 0)
    {
        sdcard_deselect (dev);
        return 0;
    }

    crc = sdcard_crc16 (0, buffer, SDCARD_BLOCK_SIZE);
    
    /* Send data start block token.  */
    command[0] = 0xFE;
    spi_write (dev->spi, command, 1, 0);

    /* Send the data.  */
    spi_write (dev->spi, buffer, SDCARD_BLOCK_SIZE, 0);

    command[0] = crc >> 8;
    command[1] = crc & 0xff;

    /* Send the crc.  */
    spi_write (dev->spi, command, 2, 0);

    /* Get the status response.  */
    command[0] = 0xff;
    spi_transfer (dev->spi, command, response, 1, 0);    
    
    /* Check to see if the data was accepted.  */
    if ((response[0] & 0x1F) != SD_WRITE_OK)
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

    return SDCARD_BLOCK_SIZE;
}


uint16_t
sdcard_block_write (sdcard_t dev, sdcard_addr_t addr, const void *buffer)
{
    int page;
    int first_block;
    sdcard_addr_t addr1;
    sdcard_addr_t addr2;
    sdcard_addr_t new_page_addr;
    sdcard_addr_t spare_page_addr = 1000 * SDCARD_PAGE_SIZE;
    int i;

    page = addr / SDCARD_PAGE_SIZE;
    first_block = page * SDCARD_PAGE_SIZE;
    new_page_addr = first_block * SDCARD_PAGE_SIZE;

    /* Erase spare page.  */
    sdcard_page_erase (dev, spare_page_addr);

    /* Copy data to spare page.  */
    addr1 = new_page_addr;
    addr2 = spare_page_addr;
    for (i = 0; i < SDCARD_PAGE_BLOCKS; i++)
    {
        int bytes;
        uint8_t tmp[SDCARD_BLOCK_SIZE];

        bytes = sdcard_block_read (dev, addr1, tmp);
        bytes = sdcard_erased_block_write (dev, addr2, tmp);
        addr1 += SDCARD_BLOCK_SIZE;
        addr2 += SDCARD_BLOCK_SIZE;
    }
    
    /* Erase desired page.  */
    sdcard_page_erase (dev, addr1);

    /* Copy data from spare page.  */
    addr1 = new_page_addr;
    addr2 = spare_page_addr;
    for (i = 0; i < SDCARD_PAGE_BLOCKS; i++)
    {
        int bytes;
        uint8_t tmp[SDCARD_BLOCK_SIZE];
        
        if (addr2 == addr)
        {
            bytes = sdcard_erased_block_write (dev, addr1, buffer);
        }
        else
        {
            bytes = sdcard_block_read (dev, addr2, tmp);
            bytes = sdcard_erased_block_write (dev, addr1, tmp);
        }
        addr1 += SDCARD_BLOCK_SIZE;
        addr2 += SDCARD_BLOCK_SIZE;
    }
    return SDCARD_BLOCK_SIZE;
}


sdcard_ret_t
sdcard_write (sdcard_t dev, sdcard_addr_t addr, const void *buffer,
              sdcard_size_t size)
{
    uint16_t blocks;
    uint16_t i;
    sdcard_size_t total;
    sdcard_size_t bytes;
    const uint8_t *src;

    /* Ignore partial writes.  */
    if (addr % SDCARD_BLOCK_SIZE || size % SDCARD_BLOCK_SIZE)
        return 0;

    blocks = size / SDCARD_BLOCK_SIZE;
    src = buffer;
    total = 0;
    for (i = 0; i < blocks; i++)
    {
        bytes = sdcard_block_write (dev, addr + i, src);
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

    /* Send the card 80 clocks to activate it (at least 74 are
       required).  */
    spi_cs_disable (dev->spi);
    spi_write (dev->spi, dummy, sizeof (dummy), 1);
    spi_cs_enable (dev->spi);

    /* Send software reset.  */
    status = sdcard_command (dev, SD_OP_GO_IDLE_STATE, 0);

    sdcard_deselect (dev);

    if (status != 0x01)
        return SDCARD_ERR_NO_CARD;

    /* Check to see if card happy with our supply voltage.  */

    for (retries = 0; retries < 256; retries++)
    {
        /* Need to keep sending this command until the in-idle-state
           bit is set to 0.  */
        status = sdcard_command (dev, SD_OP_SEND_OP_COND, 0);
        if ((status & 0x01) == 0)
            break;
    }

    sdcard_deselect (dev);

    if (status != 0)
    {
        /* Have an error bit set.  */
        return SDCARD_ERR_ERROR;
    }

#if 0
    /* Read operation condition register (OCR).  */
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
#endif
    
    status = sdcard_command (dev, SD_OP_SET_BLOCKLEN, SDCARD_BLOCK_SIZE);
    sdcard_deselect (dev);

    if (status != 0)
        return SDCARD_ERR_ERROR;

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

    /* Ensure chip select isn't asserted too soon.  */
    spi_cs_assert_delay_set (dev->spi, 16);    
    /* Ensure chip select isn't negated too soon.  */
    spi_cs_negate_delay_set (dev->spi, 16);    
   
    return dev;
}


void
sdcard_shutdown (sdcard_t dev)
{
    // TODO
    spi_shutdown (dev->spi);
}

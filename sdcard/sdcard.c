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

enum {SD_BLOCK_SIZE = 512};
enum {SD_CMD_LEN = 6};

typedef enum 
{
    SD_OP_SEND_CSD = 9,           /* CMD9 */
    SD_OP_READ_SINGLE_BLOCK = 17, /* CMD17 */
    SD_OP_WRITE_BLOCK = 24,     /* CMD24 */
    SD_OP_WRITE_MULTIPLE_BLOCK = 25, /* CMD25 */
    SD_OP_CRC_ON_OFF = 59,           /* CMD59 */
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
sdcard_crc16 (uint16_t crc, void *bytes, uint8_t size)
{
    uint8_t i;
    uint8_t *data = bytes;
    
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
sdcard_crc7 (uint8_t crc, void *bytes, uint8_t size)
{
    uint8_t i;
    uint8_t *data = bytes;

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
        spi_read (dev->spi, &response, 1, 1);
        if (response == desired)
            return 1;
    }
    return 0;
}


static uint8_t
sdcard_command (sdcard_t dev, sdcard_op_t op, uint32_t param)
{
    uint8_t command[SD_CMD_LEN];

    command[0] = op | SD_HOST_BIT;
    command[1] = param >> 24;
    command[2] = param >> 16;
    command[3] = param >> 8;
    command[4] = param;
    command[5] = (sdcard_crc7 (command, 5) << 1) | SD_END_BIT

    spi_write (dev->spi, command, SD_CMD_LEN, 1);

    /* Should get a R1 response.  */

    
    return 0;
}


// Write a 512 byte block at a given location (according to Sandisk SD
// card product manual, 512 bytes is minimum block len see pp5-1)
uint16_t
sdcard_write_block (sdcard_t dev, void *buffer, sdcard_block_t block)
{

    uint8_t status;
    uint16_t checksum;
    sdcard_addr_t addr;
    uint8_t *src = buffer;

    addr = block * SD_BLOCK_SIZE;

    sdcard_command (dev, SD_OP_WRITE_BLOCK, addr);
    
    if (!sdcard_response_match (0x00))
    {
        pio_output_high (cfg->cs);
        // Response match failed
        return SD_WRITE_CMD_FAIL;
    }
    
    // Calculate Checksum
    checksum = CRC_16Bit (src, SD_BLOCK_SIZE);
    
    // Send data begin token (bit 0 = 0)
    SPI1Byte (0xFE);
    // Send the data
    spi_write (dev->spi, src, SD_BLOCK_SIZE, 1);
    // Send checksum bytes
    SPI1Byte (checksum >> 8);
    SPI1Byte (checksum & 0xFF);
    
    // Send dummy data to get response info
    status = SPI1Byte (0xFF);
    
    // Check to see if Data was accepted
    if ((status & 0x0F) != SD_DATA_WRITE_OK)
    {
        pio_output_high (cfg->cs);
        
        // See pp5-15 Sandisk SD product man (rev 1.9)
        if ((status& 0x0F) == 11)
            return SD_WRITE_CRC_FAIL;

        return SD_WRITE_DATA_FAIL;
    }
    
    // Now wait for card to complete write cycle
    if (!sdcard_response_match (0x00))
    {
        pio_output_high (cfg->cs);
        return SD_WRITE_RESPONSE_FAIL;
    }
    
    pio_output_high (cfg->cs);
    
    SPI1Byte (0xFF);
    return SD_WRITE_OK;
}


// UNTESTED
sdcard_ret_t
sdcard_read_part_block (sdcard_t dev, void *buffer, sdcard_block_t block, 
                        uint16_t offset, sdcard_size_t len)
{
    uint16_t CheckRcv;
    uint8_t *dst = buffer;
    static uint8_t command[SD_CMD_LEN] = {SD_OP_READ_SINGLE_BLOCK, 0x00, 0x00, 0x00, 0x00, 0xFF};
    sdcard_addr_t addr;
    uint16_t CRC;
    uint16_t curPos;
    int i;
    
    command[0] = SD_OP_READ_SINGLE_BLOCK;
    
    // Set up leading framing bits (must be done *before* CRC)
    // TODO: (this can be statically calced)
    command[0] = (command[0] | (1 << 6)) & 0x7F;
    
    // Convert Block adressing into byte addressing
    addr = block * SD_BLOCK_SIZE;

    command[1] = addr >> 24;
    command[2] = addr >> 16;
    command[3] = addr >> 8;
    command[4] = addr;
    
    command[5] = sdcard_crc7 (command, 5) << 1 | 1;
    
    spi_write (dev->spi, command, SD_CMD_LEN, 1);
    
    // Check that we received the apropriate respnse (0 indicates no error)
    if (!sdcard_response_match (0x00))
    {
        return SD_READ_CMD_FAIL; // This could be CRC or some other error
    }
    
    // Check for incoming data token (this is SUPPOSEDLY 0xFE, but 0xFF appears to work)
    // TODO: find out why (card version wierdness? 0xFE for sandisk.... 0xFF for kodak)
    if (!sdcard_response_match (0xFE))
    {
        return SD_READ_DATA_FAIL;
    }
    
    curPos = 0;
    CRC = 0;
	
    // Use init bytes for CRCing
    while (curPos < offset)
    {
        CRC = CRC_16Bit_AddByte (CRC, SPI1Byte (0xFF));
        curPos++;
    }
    
    for (i = 0; i < len; i++)
    {
        *dst = SPI1Byte (0xFF);
        CRC = CRC_16Bit_AddByte (CRC, *dst);
        dst++;
        curPos++;
    }

    while (curPos < SD_BLOCK_SIZE)
    {
        CRC = CRC_16Bit_AddByte (CRC, SPI1Byte (0xFF));
        curPos++;
    }
    
    // Receive Checksum
    CheckRcv = SPI1Byte (0xFF) << 8;
    CheckRcv |= SPI1Byte (0xFF);
    
    if (CheckRcv != CRC)
    {
        // CRC error!
        return SD_READ_CRC_FAIL;
    }
    
    // Provide some clocks to the SD card (pp 5-6 sandisk SD product manual)
    SPI1Byte (0xFF);
    
    return len;
}


// UNTESTED
sdcard_ret_t
sdcard_write_block_head (sdcard_dev_t dev, void *buffer, sdcard_block_t block, 
                         sdcard_size_t len, uint8_t padding)
{
    // Block addr Arguments to the SD card --phillips
    uint8_t *src = buffer;
    static uint8_t command[SD_CMD_LEN] = {SD_OP_WRITE_BLOCK, 0x00, 0x00, 0x00, 0x00, 0xFF};
    uint8_t status;
    uint16_t checksum;
    uint16_t temp;
    sdcard_addr_t addr;

    // Set up leading framing bits (must be done *before* CRC)
    command[0] = (command[0] | (1 << 6)) & 0x7F;
    
    // Convert Block adressing into byte addressing
    addr = block * SD_BLOCK_SIZE;
    
    command[1] = addr >> 24;
    command[2] = addr >> 16;
    command[3] = addr >> 8;
    command[4] = addr;
    // Append command CRC
    command[5] = sdcard_crc7 (command, 5) << 1;
    
    spi_write (dev->spi, command, SD_CMD_LEN, 1);
    
    if (!sdcard_response_match (0x00))
    {
        // Response match failed
        return SD_WRITE_CMD_FAIL;
    }
    
    // Calculate Checksum
    checksum = CRC_16Bit (src, len);
    
    // Send data begin token (bit 0 = 0)
    SPI1Byte (0xFE);
    temp = 0;
    while (temp++ < len)
        SPI1Byte (* (src+temp));
    
    while (temp < SD_BLOCK_SIZE)
    {
        checksum = CRC_16Bit_AddByte (checksum, padding);
        SPI1Byte (padding);
    }
    
    // Send checksum bytes
    SPI1Byte (checksum >> 8);
    SPI1Byte (checksum & 0xFF);
    
    // Send dummy data to get response info
    status = SPI1Byte (0xFF);
    
    // Check to see if Data was accepted
    if ((status & 0x0F) != SD_DATA_WRITE_OK)
    {
        // See pp5-15 Sandisk SD product man (rev 1.9)
        if ((status & 0x0F) == 11)
            return SD_WRITE_CRC_FAIL;
        
        return SD_WRITE_DATA_FAIL;
    }
    
    // Now wait for card to complete write cycle
    if (!sdcard_response_match (0x00))
    {
        return SD_WRITE_RESPONSE_FAIL;
    }
    
    SPI1Byte (0xFF);
    
    return len;
}


// UNTESTED
sdcard_ret_t 
sdcard_write_multi_block (sdcard_t *dev, void *buffer, sdcard_block_t block, 
                          uint16_t num_blocks)
{
    // Block addr Arguments to the SD card --phillips
    static uint8_t command[SD_CMD_LEN] = {SD_OP_WRITE_MULTIPLE_BLOCK, 0x00, 0x00, 0x00, 0x00, 0xFF};
    uint8_t status;
    uint16_t checksum;
    sdcard_addr_t addr;
    uint16_t curBlock = 0;
    uint8_t *src = buffer;

    // Set up leading framing bits (must be done *before* CRC)
    command[0] = (command[0] | (1 << 6)) & 0x7F;
    
    // Convert Block adressing into byte addressing
    addr = block * SD_BLOCK_SIZE;
    
    command[1] = addr >> 24;
    command[2] = addr >> 16;
    command[3] = addr >> 8;
    command[4] = addr;
    // Append command CRC
    command[5] = sdcard_crc7 (command, 5) << 1;
    
    spi_write (dev->spi, command, SD_CMD_LEN, 1);
    
    if (!sdcard_response_match (0x00))
    {
        // Response match failed
        return (uint16_t) (-1);
    }
    
    while (curBlock < num_blocks)
    {
        // Calculate Checksum
        checksum = CRC_16Bit (src, SD_BLOCK_SIZE);
        // Send data token "start transmission"
        SPI1Byte (0xFC);
	
        // Send block
        spi_write (dev->spi, src, SD_BLOCK_SIZE, 1);
        src+=SD_BLOCK_SIZE;
        // Send checksum bytes
        SPI1Byte (checksum >> 8);
        SPI1Byte (checksum & 0xFF);		
	
        status = SPI1Byte (0xFF);
        if ((status & 0x0F) != SD_DATA_WRITE_OK)
        {
            if (curBlock)
                return curBlock;
            else
                return (uint16_t) -1;
        }
	
        // Send dummy data to get response info
        status = SPI1Byte (0xFF);
        
        // Now wait for card to complete write cycle
        if (!sdcard_response_match (0x00))
        {
            if (curBlock)
                return curBlock;
            else
                return (uint16_t) -1;
            
        }
        curBlock++;
    }
    
    // Send stop token
    SPI1Byte (0xFD);
    
    sdcard_response_match (0x00);
    
    return num_blocks * SD_BLOCK_SIZE;
}


// Read a 512 block of data on the SD card
sdcard_ret_t 
sdcard_read_block (sdcard_t dev, void *buffer, sdcard_block_t block)
{
    uint16_t CheckRcv;
    static uint8_t command[SD_CMD_LEN] = {SD_OP_READ_SINGLE_BLOCK, 0x00, 0x00, 0x00, 0x00, 0xFF};
    sdcard_addr_t addr;    
    uint8_t *dst = buffer;

    command[0] = SD_OP_READ_SINGLE_BLOCK;
    
    // Set up leading framing bits (must be done *before* CRC)
    // TODO: (this can be statically calced)
    command[0] = (command[0] | (1 << 6)) & 0x7F;
    
    // Convert Block adressing into byte addressing
    addr = block * SD_BLOCK_SIZE;
    
    command[1] = addr >> 24;
    command[2] = addr >> 16;
    command[3] = addr >> 8;
    command[4] = addr;
    
    command[5] = sdcard_crc7 (command, 5) << 1 | 1;
    
    spi_write (dev->spi, command, SD_CMD_LEN, 1);
    
    // Check that we received the apropriate respnse (0 indicates no error)
    if (!sdcard_response_match (0x00))
    {
        return SD_READ_CMD_FAIL; // This could be CRC or some other error
    }
    
    // Check for incoming data token (this is SUPPOSEDLY 0xFE, but 0xFF appears to work)
    // TODO: find out why (card version wierdness? 0xFE for sandisk.... 0xFF for kodak)
    if (!sdcard_response_match (0xFE))
    {
        return SD_READ_DATA_FAIL;
    }
    
    // Receive data
    SPI1Receive (dst, SD_BLOCK_SIZE);
    
    // Receive checksum
    CheckRcv = SPI1Byte (0xFF) << 8;
    CheckRcv |= SPI1Byte (0xFF);
    
    if (CheckRcv != CRC_16Bit (dst, SD_BLOCK_SIZE))
    {
        // CRC error!
        return SD_READ_CRC_FAIL;
    }
    
    // Provide some clocks to the SD card (pp 5-6 sandisk SD product manual)
    SPI1Byte (0xFF);
    
    return SD_BLOCK_SIZE;
}


// UNTESTED
sdcard_ret_t
sdcard_read_block_head (sdcard_t dev, void *buffer, sdcard_block_t block, 
                        sdcard_size_t len)
{
    uint16_t CheckRcv, tmp, runSum;
    static uint8_t command[SD_CMD_LEN] = {SD_OP_READ_SINGLE_BLOCK, 0x00, 0x00, 0x00, 0x00, 0xFF};
    sdcard_addr_t addr;
    uint8_t *dst = buffer;
    
    command[0] = SD_OP_READ_SINGLE_BLOCK;
    
    // Set up leading framing bits (must be done *before* CRC)
    // TODO: (this can be statically calced)
    command[0] = (command[0] | (1 << 6)) & 0x7F;

    // Convert Block adressing into byte addressing
    addr = block * SD_BLOCK_SIZE;
    
    command[1] = addr >> 24;
    command[2] = addr >> 16;
    command[3] = addr >> 8;
    command[4] = addr;
    
    command[5] = sdcard_crc7 (command, 5) << 1 | 1;
    spi_write (dev->spi, command, SD_CMD_LEN, 1);
    
    // Check that we received the apropriate respnse (0 indicates no error)
    if (!sdcard_response_match (0x00))
    {
        return SD_READ_CMD_FAIL; // This could be CRC or some other error
    }
    
    // Check for incoming data token 
    // TODO: find out why sometimes 0xFE sometimes 0xFF (card version wierdness? 0xFE for sandisk.... 0xFF for kodak)
    if (!sdcard_response_match (0xFE))
    {
        return SD_READ_DATA_FAIL;
    }
    
    // Receive data
    tmp = 0;
    while (tmp < len)
    {
        * (dst+tmp) = SPI1Byte (0xFF);
        tmp++;
    }
    runSum = CRC_16Bit (dst, tmp);
    
    while (tmp < SD_BLOCK_SIZE)
    {
        runSum = CRC_16Bit_AddByte (runSum, SPI1Byte (0xFF));
        tmp++;
    }
    // Receive checksum
    CheckRcv = SPI1Byte (0xFF) << 8;
    CheckRcv |= SPI1Byte (0xFF);
    
    if (CheckRcv != runSum)
    {
        // CRC error!
        return SD_READ_CRC_FAIL;
    }
    
    // Provide some clocks to the SD card (pp 5-6 sandisk SD product manual)
    SPI1Byte (0xFF);
    
    return len;
}


//  Activates or disables CRC checking on SD card (0 = off 1 = on)
uint16_t sdcard_crc_enable (sdcard_t dev, uint16_t CRC)
{
    static uint8_t command[SD_CMD_LEN] = {0x3B, 0x00, 0x00, 0x00, 0x00, 0xFF};
    uint8_t CRC;

    command[0] = 0x3B;
    command[4] = CRC & 0x01;
    
    // TODO: Can optimise this by examining two cases for CRC and recording them..
    // Calculate CRC and framing bits
    command [0] = (command[0] | (1 << 6)) & 0x7F;
    CRC = sdcard_crc7 (command, 5);
    command[5] = CRC << 1 | 1;
    
    spi_write (dev->spi, command, SD_CMD_LEN, 1);
    
    // Check that we received the apropriate response (0 indicates no error)
    if (!sdcard_response_match (0x00))
    {
        return SD_CMD_FAIL; // This could be CRC or some other error
    }
    
    // Provide some clocks to the SD card (pp 5-6 sandisk SD product manual)
    SPI1Byte (0xFF);
    return 0;
}


// CSD is 16bytes
// Returns 0 on success, nonzero on fail
uint16_t SDGetCSDRegister (sdcard_t dev, uint8_t *cpBuffer)
{
    static uint8_t command[SD_CMD_LEN] = {SD_OP_SEND_CSD, 0x00, 0x00, 0x00, 0x00, 0xFF};

    command[0] = 0x09;
    
    // Set up leading framing bits (must be done *before* CRC)
    // TODO: (this can be statically calced)
    command[0] = (command[0] | (1 << 6)) & 0x7F;
    
    // Set up trailing CRC byte (7 bit crc followed by stop bit)
    command[5] = sdcard_crc7 (command, 5) << 1 | 1;
    
    spi_write (dev->spi, command, SD_CMD_LEN, 1);
    
    // Wait for appropriate response token (sd.c from efs project)
    if (!sdcard_response_match (0xFE))
    {
        return SD_GET_CSD_FAIL;
    }
    
    SPI1Receive (cpBuffer, 16);
    
    return 0;
}


uint16_t SDCardGetBlockcount (sdcard_t dev)
{
    uint8_t raw_csd[16];
    uint16_t c_size;
    uint16_t c_size_mult;
    
    if (SDGetCSDRegister (dev, raw_csd))
        return 0;
    
    c_size = ((((uint16_t)raw_csd[6]) & 0x03) << 10)
        | (((uint16_t)raw_csd[7]) << 2)
        | (((uint16_t)raw_csd[8]) & 0xc0) >> 6;

    c_size_mult = ((raw_csd[9] & 0x03) << 1) | ((raw_csd[10] & 0x80) >> 7);

    return ((uint16_t) (c_size+1)) * (1 << (c_size_mult + 2));
}


uint16_t SDCardGetBlockSize (sdcard_t dev)
{
    uint8_t raw_csd[16];
    
    if (SDGetCSDRegister (dev, raw_csd))
        return 0;
    
    return 1 << (raw_csd[5] & 0x0f);
}


sdcard_t
sdcard_init (sdcard_obj_t *obj, const sdcard_cfg_t *cfg)
{
    sdcard_t dev;
    uint16_t page_size;
    static uint8_t idle_command[SD_CMD_LEN] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
    static uint8_t SDCmd_GetOpCond[SD_CMD_LEN] = {0x41, 0x00, 0x00, 0x00, 0x00, 0xFF};
    uint8_t dummy[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint16_t count;

    if (sdcard_devices_num >= SDCARD_DEVICES_NUM)
        return 0;

    dev = sdcard_devices + sdcard_devices_num;

    page_size = cfg->page_size;
    dev->cfg = cfg;
    dev->page_bits = 0;

    while (page_size)
    {
        page_size >>= 1;
        dev->page_bits++;
    }

    dev->size = cfg->pages * cfg->page_size;

    dev->spi = spi_init (&cfg->spi);

    // Hmmm, should we let the user override the mode?
    spi_mode_set (dev->spi, SPI_MODE_0);
    spi_cs_mode_set (dev->spi, SPI_CS_MODE_FRAME);
    /* Ensure chip select isn't asserted too soon.  */
    spi_cs_assert_delay_set (dev->spi, 16);    
    /* Ensure chip select isn't negated too soon.  */
    spi_cs_negate_delay_set (dev->spi, 16);    

    if (cfg->wp.port)
    {
        pio_config_set (cfg->wp, PIO_OUTPUT);
        pio_output_low (cfg->wp);
    }

    // Send the card clocks to activate it
    spi_write (dev->spi, dummy, sizeof (dummy), 1);
    
    // Send CMD0
    spi_write (dev->spi, idle_command, SD_CMD_LEN, 1);
    
    // Wait for idle state return byte
    if (!sdcard_response_match (R1_IN_IDLE_STATE))
    {
        return SD_WAKE_FAIL;
    }
    
    // At this point the card is now in its IDLE state

    // Send some dummy clocks to allow SD card to finish task
    SPI1Byte (0xFF);
    
    // Now bring card up to full power
    
    // Repeat the command sequence until the SD card reaches full power
    count = SD_CMD_REPEAT_MAX;
    do
    {
        spi_write (dev->spi, SDCmd_GetOpCond, SD_CMD_LEN, 1);
    } while (!sdcard_response_match (0x00) && --count);
    
    if (!count)
    {
        return SD_INIT_FAIL;
    }
    
    // Send some more dummy clocks
    SPI1Byte (0xFF);
    
    // Generate the CRC table
    CRC16Bit_GenTable ();
    
    
    // sdcard_status_read (dev);
    
    return dev;
}


void
sdcard_shutdown (sdcard_t dev)
{
    uint8_t command[1];

    command[0] = SDCARD_OP_POWERDOWN;

    spi_write (dev->spi, command, sizeof (command), 1);
    spi_shutdown (dev->spi);
}

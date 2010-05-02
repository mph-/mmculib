/** @file   sdcard.c
    @author Michael Hayes
    @date   1 May 2010
    @brief  Secure digital card driver.  */

#include <string.h>
#include "sdcard.h"
#include "delay.h"

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
    SD_OP_SEND_IF_COND = 8,           /* CMD8 */
    SD_OP_SEND_CSD = 9,               /* CMD9 */
    SD_OP_SEND_CID = 10,              /* CMD10 */
    SD_OP_SEND_STATUS = 13,           /* CMD13 */
    SD_OP_SET_BLOCKLEN = 16,          /* CMD16 */
    SD_OP_READ_SINGLE_BLOCK = 17,     /* CMD17 */
    SD_OP_WRITE_BLOCK = 24,           /* CMD24 */
    SD_OP_WRITE_MULTIPLE_BLOCK = 25,  /* CMD25 */
    SD_OP_ERASE_WR_BLK_START_ADDR = 32,
    SD_OP_ERASE_WR_BLK_END_ADDR = 33,
    SD_OP_ERASE = 38,
    SD_OP_APP_SEND_OP_COND = 41,      /* ACMD41 */
    SD_OP_APP_CMD = 55,               /* CMD55 */
    SD_OP_READ_OCR = 58,              /* CMD58 */
    SD_OP_CRC_ON_OFF = 59,            /* CMD59 */
} sdcard_op_t;


enum 
{
    SD_WRITE_OK = 5,
    SD_WRITE_CRC_ERROR = 11,
    SD_WRITE_ERROR = 13
};


enum
{
    SD_START_TOKEN = 0xfe
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

#define SDCARD_NCR 8



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
sdcard_crc16 (uint16_t crc, const void *buffer, uint16_t size)
{
    uint8_t i;
    const uint8_t *data = buffer;
    
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
sdcard_crc7 (uint8_t crc, const void *buffer, uint8_t size)
{
    uint8_t i;
    const uint8_t *data = buffer;

    for (i = 0; i < size; i++)
        crc = sdcard_crc7_byte (crc, data[i], 8);

    crc = sdcard_crc7_byte (crc, 0, 7);

    return crc;
}


/*  Keeps polling the SD card until the desired byte is returned.  */
bool
sdcard_response_match (sdcard_t dev, uint8_t desired, uint32_t timeout)
{
    uint16_t retries;
    uint8_t command[1];
    uint8_t response[1];

    command[0] = 0xff;    

    /* Keep reading the SD card for the desired response or timeout.  */
    for (retries = 0; retries < timeout + 1; retries++)
    {
        spi_transfer (dev->spi, command, response, 1, 0);
        if (response[0] == desired)
            return 1;
    }
    return 0;
}


/*  Keeps polling the SD card until not the desired byte is returned.  */
bool
sdcard_response_not_match (sdcard_t dev, uint8_t desired, uint32_t timeout)
{
    uint16_t retries;
    uint8_t command[1];
    uint8_t response[1];

    command[0] = 0xff;
    
    /* Keep reading the SD card for the desired response or timeout.  */
    for (retries = 0; retries < timeout + 1; retries++)
    {
        spi_transfer (dev->spi, command, response, 1, 0);
        if (response[0] != desired)
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

    spi_transfer (dev->spi, dummy, dummy, sizeof (dummy), 1);

    spi_cs_enable (dev->spi);
}


static uint8_t
sdcard_command (sdcard_t dev, sdcard_op_t op, uint32_t param)
{
    uint8_t command[SD_CMD_LEN];
    uint8_t response[SD_CMD_LEN];
    uint16_t retries;
    
    /* Wait N_cs clock cyles, min is 0.  */
    command[0] = 0xff;
    spi_transfer (dev->spi, command, response, 1, 0);

    command[0] = op | SD_HOST_BIT;
    command[1] = param >> 24;
    command[2] = param >> 16;
    command[3] = param >> 8;
    command[4] = param;

#if 0
    command[5] = (sdcard_crc7 (0, command, 5) << 1) | SD_STOP_BIT;
#else
    switch (op)
    {
        /* The CRC only needs to be calculated for two commands in SPI
           mode.  */
    case SD_OP_GO_IDLE_STATE:
        command[5] = 0x95;
        break;

    case SD_OP_SEND_IF_COND:
        command[5] = 0x86;
        break;

    default:
        command[5] = 0xff;
        break;
    }
#endif
        
    /* Send command; the card will respond with a sequence of 0xff.  */
    spi_transfer (dev->spi, command, response, SD_CMD_LEN, 0);

    command[0] = 0xff;

    /* Search for R1 response; the card should respond with 0 to 8
       bytes of 0xff beforehand.  */
    for (retries = 0; retries < SDCARD_NCR + 1; retries++)
    {
        spi_transfer (dev->spi, command, response, 1, 0);

        if (op == SD_OP_GO_IDLE_STATE)
        {
            if (response[0] == 0x01)
                break;
        }
        else
        {
            if (response[0] != 0xff)
                break;
        }
    }

    dev->status = response[0];
    return dev->status;
}


uint8_t
sdcard_app_command (sdcard_t dev, sdcard_op_t op, uint32_t param)
{
    uint8_t status;

    /* Send CMD55.  */
    status = sdcard_command (dev, SD_OP_APP_CMD, 0);
    sdcard_deselect (dev);

    /* Should get 0x00 or 0x01 response.  Anything else is an error.  */

    status = sdcard_command (dev, op, param);
    return status;
}


uint16_t
sdcard_block_size_set (sdcard_t dev, uint16_t bytes)
{
    uint8_t status;

    if (dev->type != SDCARD_TYPE_SD)
        return 0;

    status = sdcard_command (dev, SD_OP_SET_BLOCKLEN, bytes);
    sdcard_deselect (dev);
    if (status)
        return 0;

    dev->block_size = bytes;
    return bytes;
}


/* Send a command, read the R1 response, and data packet.  */
uint8_t
sdcard_command_read (sdcard_t dev, sdcard_op_t op, uint32_t param,
                     uint8_t *buffer, uint16_t bytes)
{
    uint8_t crc[2];
    uint8_t status;
    uint32_t timeout;

    status = sdcard_command (dev, op, param);
    if (status != 0)
    {
        sdcard_deselect (dev);
        return status;
    }

    switch (op)
    {
    case SD_OP_READ_SINGLE_BLOCK:
        timeout = dev->Nac;
        break;
        
    default:
        /* Use Ncr.  */
        timeout = SDCARD_NCR;
    }

    /* Wait for card to return the start data token.  */
    if (!sdcard_response_match (dev, SD_START_TOKEN, timeout))
    {
        sdcard_deselect (dev);
        return 3;
    }

    /* Read the data.  */
    memset (buffer, 0xff, bytes);
    spi_transfer (dev->spi, buffer, buffer, bytes, 0);

    /* Read the 16 bit crc.  */
    memset (crc, 0xff, sizeof (crc));
    spi_transfer (dev->spi, crc, crc, sizeof (crc), 0);

    sdcard_deselect (dev);
    
    return 0;
}


/* Send a command and read the R1 response and following bytes.  */
uint8_t
sdcard_command_response (sdcard_t dev, sdcard_op_t op, uint32_t param,
                         uint8_t *buffer, uint16_t bytes)
{
    uint8_t status;

    status = sdcard_command (dev, op, param);
    
    /* Read the rest of the response.  */
    memset (buffer, 0xff, bytes);
    spi_transfer (dev->spi, buffer, buffer, bytes, 0);
    sdcard_deselect (dev);

    return status;
}


sdcard_status_t
sdcard_status_read (sdcard_t dev)
{
    uint8_t status;
    uint8_t extra;

    status = sdcard_command_response (dev, SD_OP_SEND_STATUS, 0, &extra, 1);
    return (status << 8) | extra;
}


uint8_t
sdcard_csd_read (sdcard_t dev, uint8_t *csd, uint8_t bytes)
{
    return sdcard_command_read (dev, SD_OP_SEND_CSD, 0, csd, bytes);
}


uint8_t
sdcard_cid_read (sdcard_t dev, uint8_t *cid, uint8_t bytes)
{
    return sdcard_command_read (dev, SD_OP_SEND_CID, 0, cid, bytes);
}


uint8_t
sdcard_cmd8 (sdcard_t dev)
{
    uint8_t buffer[4];
    uint8_t status;

    /* This command is not supported by older cards.  */

    status = sdcard_command_response (dev, SD_OP_SEND_IF_COND, 0x1aa,
                                      buffer, sizeof (buffer));

    /* buffer[2] should be 0x1 and buffer[3] should be 0xaa.  */
    /* status should be 1.  */

    return status;
}


uint8_t
sdcard_ocr_read (sdcard_t dev, uint32_t *pocr)
{
    uint8_t buffer[4];
    uint8_t status;

    /* Read operation condition register (OCR).  */
    status = sdcard_command_response (dev, SD_OP_READ_OCR, 0,
                                      buffer, sizeof (buffer));
    if (status > 1)
        return status;

    *pocr = (buffer[0] << 24) | (buffer[1] << 16)
        | (buffer[2] << 8) | buffer[3];
    
    /* Before initialisation I get 0x00ff8000.  This is what the
       SanDisk data sheet says for 2.7--3.6 V operation.  The MSB (Bit
       31) is 0 so the power up procedure is not finished.

       After initialisation I get 0xc0ff8000.  This has the CCS bit
       set.  */

    return status;
}


uint8_t
sdcard_init_poll (sdcard_t dev)
{
    uint8_t status;

    /* Activate card's initialisation process and poll for finished.  */

#if 1
    /* Send CM55/ACMD41 with set high capacity support bit set.
       Note older cards require SD_OP_SEND_OP_COND.  */
    status = sdcard_app_command (dev, SD_OP_APP_SEND_OP_COND, BIT (30));
#else
    status = sdcard_command (dev, SD_OP_SEND_OP_COND, 0);
#endif

    sdcard_deselect (dev);

    /* It appears that if the power supply current limits the card
       will just send a continuous stream of 0x00 from this point.
       Presumably sending the initialisation command fires up a charge
       pump and the increased current draw causes thins to go awry.  */

    return status;
}


uint8_t
sdcard_init_wait (sdcard_t dev)
{
    int retries;
    uint8_t status;

    for (retries = 0; retries < 50; retries++)
    {
        status = sdcard_init_poll (dev);
        if ((status & 0x01) == 0)
            break;
        delay_ms (10);
    }

    sdcard_deselect (dev);

    return status;
}


sdcard_addr_t
sdcard_capacity (sdcard_t dev)
{
    return dev->sectors * dev->block_size;
}


bool
sdcard_page_erase (sdcard_t dev, sdcard_addr_t addr)
{
    uint8_t status;

    status = sdcard_command (dev, SD_OP_ERASE_WR_BLK_START_ADDR, addr);
    status = sdcard_command (dev, SD_OP_ERASE_WR_BLK_END_ADDR, addr 
        + SDCARD_PAGE_SIZE);

#if 0
    status = sdcard_command (dev, SD_OP_ERASE, 0);
    /* Need to wait for busy to clear.  */
#endif

    return 1;
}


sdcard_ret_t 
sdcard_block_read (sdcard_t dev, sdcard_addr_t addr, void *buffer)
{
    uint8_t status;

    status = sdcard_command_read (dev, SD_OP_READ_SINGLE_BLOCK,
                                  addr >> dev->addr_shift, buffer,
                                  dev->block_size);
    if (status)
        return 0;

    return dev->block_size;
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
    if (addr % dev->block_size || size % dev->block_size)
        return 0;

    blocks = size / dev->block_size;
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
sdcard_block_write (sdcard_t dev, sdcard_addr_t addr, const void *buffer)
{
    uint8_t status;
    uint16_t crc;
    uint8_t command[3];
    uint8_t response[3];

    status = sdcard_command (dev, SD_OP_WRITE_BLOCK, addr >> dev->addr_shift);
    if (status != 0)
    {
        sdcard_deselect (dev);
        return 0;
    }

#if 0
    crc = sdcard_crc16 (0, buffer, dev->block_size);
#else
    crc = 0xffff;
#endif
    
    /* Send Nwr dummy clocks then data start block token.  */
    command[0] = 0xff;
    command[1] = SD_START_TOKEN;
    spi_write (dev->spi, command, 2, 0);

    /* Send the data.  */
    spi_write (dev->spi, buffer, dev->block_size, 0);

    command[0] = crc >> 8;
    command[1] = crc & 0xff;
    command[2] = 0xff;

    /* Send the crc and get the status response.  */
    spi_transfer (dev->spi, command, response, 3, 0);

    /* Check to see if the data was accepted.  */
    if ((response[2] & 0x1F) != SD_WRITE_OK)
    {
        sdcard_deselect (dev);
        return 0;
    }
    
    /* Wait for card to complete write cycle.  */
    if (!sdcard_response_not_match (dev, 0xff, dev->Nbs))
    {
        sdcard_deselect (dev);
        return 0;
    }
    
    sdcard_deselect (dev);

    /* Look for a write error; should flag the type of error.  */
    if (sdcard_status_read (dev))
        return 0;
    
    return dev->block_size;
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
    if (addr % dev->block_size || size % dev->block_size)
        return 0;

    blocks = size / dev->block_size;
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


int
sdcard_test (sdcard_t dev)
{
    sdcard_addr_t addr;
    uint8_t tmp[SDCARD_BLOCK_SIZE];
    unsigned int i;

    if (!dev->block_size)
        return 0;

    addr = (dev->sectors - 1) * dev->block_size;

    for (i = 0; i < sizeof (tmp); i++)
        tmp[i] = 0;

    if (sdcard_read (dev, addr, tmp, sizeof (tmp))
        != dev->block_size)
        return 1;

    for (i = 0; i < sizeof (tmp); i++)
        tmp[i] = i & 0xff;

    if (sdcard_write (dev, addr, tmp, sizeof (tmp))
        != dev->block_size)
        return 2;

    for (i = 0; i < sizeof (tmp); i++)
        tmp[i] = 0;

    if (sdcard_read (dev, addr, tmp, sizeof (tmp))
        != dev->block_size)
        return 3;
        
    for (i = 0; i < sizeof (tmp); i++)
    {
        if (tmp[i] != (i & 0xff))
            return 4;
    }

    for (i = 0; i < sizeof (tmp); i++)
        tmp[i] = ~(i & 0xff);

    if (sdcard_write (dev, addr, tmp, sizeof (tmp))
        != dev->block_size)
        return 5;

    for (i = 0; i < sizeof (tmp); i++)
        tmp[i] = 0;

    if (sdcard_read (dev, addr, tmp, sizeof (tmp))
        != dev->block_size)
        return 6;

    for (i = 0; i < sizeof (tmp); i++)
    {
        if (tmp[i] != ~(i & 0xff))
            return 7;
    }
    return 0;
}


static bool
sdcard_csd_parse (sdcard_t dev)
{
    uint8_t csd[16];
    uint32_t c_size;
    uint16_t read_bl_len;
    uint32_t speed;
    int i;
    static const uint8_t MMC_m[] = {10, 12, 13, 15, 20, 25, 30,
                                    35, 40, 45, 50, 55, 60, 70, 80};

    if (sdcard_csd_read (dev, csd, sizeof (csd)))
        return 0;

    read_bl_len = csd[5] & 0x0f;
    /* This must be 9 on SDHC cards. */

    dev->type = SDCARD_TYPE_SDHC;

    c_size = ((uint32_t)(csd[7] & 0x3F) << 16)
        | ((uint32_t)csd[8] << 8) | csd[9];
   
    dev->block_size = 1 << read_bl_len;
    dev->sectors = (c_size + 1) << 10;
    dev->addr_shift = 9;

    speed = 1;
    for (i = csd[3] & 0x07; i > 0; i--)
        speed *= 10;

    /*  (TRAN_SPEED_tu * 10) * (TRAN_SPEED_tv * 10) * 10000 (Hz).  */
    speed *= (MMC_m[((csd[3] >> 3) & 0x0F) - 1]) * 10000;

    dev->speed = speed;
    speed = spi_clock_speed_set (dev->spi, speed);

    /* Set Nac to 100 ms.  */
    dev->Nac = (speed / 10) / 8;

    /* Set Nbs to 250 ms.  */
    dev->Nbs = (dev->Nac * 25) / 10;

    if (c_size > 0x00ffff)
    {
        /* If capacity is bigger than 32GB, its a SDXC-card.  */
        dev->type = SDCARD_TYPE_SDXC;

        /* Set Nbs to 500 ms.  */
        dev->Nbs *= 2;
    }

    return 1;
}


sdcard_err_t
sdcard_probe (sdcard_t dev)
{
    uint8_t status;
    uint8_t dummy[10] = {0xff, 0xff, 0xff, 0xff, 0xff,
                         0xff, 0xff, 0xff, 0xff, 0xff};
    uint32_t ocr;

#if 0
    /* MMC cards need to be initialised with a clock slower than 400 kHz.  */
    spi_clock_speed_set (dev->spi, 300e3);
#endif

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

    /* Detect SD2.0 and MMC4.x cards, since CMD8 is not supported by
       SD1.x and MMC3.x cards and will return a param error.  */
    status = sdcard_cmd8 (dev);

    status = sdcard_init_wait (dev);
    if (status != 0)
    {
        /* Have an error bit set.  */
        return SDCARD_ERR_ERROR;
    }

    status = sdcard_ocr_read (dev, &ocr);

    /* If the OCR is zero then it is likely that the card is being current 
       limited and is throwing a wobbly.  */
    if (!ocr)
        return SDCARD_ERR_ERROR;

    sdcard_csd_parse (dev);

    return SDCARD_ERR_OK;
}


sdcard_t
sdcard_init (const sdcard_cfg_t *cfg)
{
    sdcard_t dev;

    if (sdcard_devices_num >= SDCARD_DEVICES_NUM)
        return 0;
    dev = sdcard_devices + sdcard_devices_num;

    memset (dev, 0, sizeof (dev));
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
   
    /* This will change when CSD read.  */
    dev->Nac = 8;

    return dev;
}


void
sdcard_shutdown (sdcard_t dev)
{
    // TODO
    spi_shutdown (dev->spi);
}

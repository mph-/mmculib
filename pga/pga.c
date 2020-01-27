/** @file  pga.c
    @author Michael Hayes
    @date   8 February 2005
    @brief Interface routines for Microchip PGAs (MCP6S2x)
    @note  This has been superseded by spi_pga.
*/

#define PGA_TRANSPARENT

#include "pga.h"

/* The bottom bit is the register select, 0 for gain, 1 for channel.  */
enum {PGA_GAIN_REGISTER = 0,
      PGA_CHANNEL_REGISTER = 1};
      
enum {PGA_INSN_NOP = 0, 
      PGA_INSN_SHUTDOWN = 1,
      PGA_INSN_WRITE = 2};


/* The PGA can be interfaced in SPI 0,0 mode (mode 0, where SCK
   normally low and slave samples on SCK rising) or SPI 1,1 mode (mode
   3, where SCK normally high and slave samples on SCK rising).
   The max SPI clock frequency is 10\,MHz.  */


#define PGA_INSN_REGISTER_WRITE(REG) ((PGA_INSN_WRITE << 5) | (REG))

#define PGA_INSN_GAIN_REGISTER_WRITE PGA_INSN_REGISTER_WRITE (PGA_GAIN_REGISTER)
#define PGA_INSN_CHANNEL_REGISTER_WRITE PGA_INSN_REGISTER_WRITE (PGA_CHANNEL_REGISTER)


/* Send a 16-bit command to the PGA.  Valid PGA commands must always
   consist of a multiple of 16 bits, sent while the chip is
   selected.  */
static void 
pga_send_command (pga_t pga, 
                  uint8_t command_byte_1,
                  uint8_t command_byte_2)
{
    uint8_t command[2];

    command[0] = command_byte_1;
    command[1] = command_byte_2;

    spi_write (pga->spi, command, 2, 1);
}


/* Change the PGA gain.  */
void 
pga_gain_set (pga_t pga, pga_gain_t gain)
{
    /* Send command to PGA.  */
    pga_send_command (pga, PGA_INSN_GAIN_REGISTER_WRITE, gain);
}


/* Change the PGA channel.  */
void 
pga_channel_set (pga_t pga, pga_channel_t channel)
{
    /* Send command to PGA.  */
    pga_send_command (pga, PGA_INSN_CHANNEL_REGISTER_WRITE, channel);
}


void
pga_shutdown (pga_t pga)
{
    /* Send shutdown command to PGA.  */
    pga_send_command (pga, PGA_INSN_SHUTDOWN, 0);  
    spi_shutdown (pga->spi);
}


void
pga_wakeup (pga_t pga)
{
    /* The only way to wake the PGA up is to send it a valid command.
       The only valid commands at this stage are register
       configuration commands, so to wake the PGA up, the channel is
       set to 0, and the gain to 1.  */
    pga_channel_set (pga, PGA_CHANNEL_0);
    pga_gain_set (pga, PGA_GAIN_1);
}


/* Initialise the PGA for operation.  PGA is a pointer into RAM that
   stores the state of the PGA.  CFG is a pointer into ROM to define
   the port the PGA is connected to.  The returned handle is passed to
   the other pga_xxx routines to denote the PGA to operate on.  */
extern pga_t
pga_init (pga_obj_t *pga, const pga_cfg_t *cfg)
{
    spi_t spi;
    
    /* Initialise spi port.  */
    spi = spi_init (cfg);
    pga->spi = spi;

    spi_bits_set (spi, 8);
    spi_mode_set (spi, SPI_MODE_3);
    spi_divisor_set (spi, PGA_SPI_DIVISOR);
    spi_cs_mode_set (spi, SPI_CS_MODE_FRAME);

    pga_wakeup (pga);    

    return pga;
}

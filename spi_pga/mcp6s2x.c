/** @file  mcp6s2x.c
    @author Michael Hayes
    @date   8 February 2005
    @brief Interface routines for Microchip MCP6S2Xs programmable gain amplifiers
*/

#include "spi_pga.h"


/* The bottom bit is the register select, 0 for gain, 1 for channel.  */
enum {MCP6S2X_GAIN_REGISTER = 0,
      MCP6S2X_CHANNEL_REGISTER = 1};
      
enum {MCP6S2X_INSN_NOP = 0, 
      MCP6S2X_INSN_SHUTDOWN = 1,
      MCP6S2X_INSN_WRITE = 2};


/* The MCP6S2X can be interfaced in SPI 0,0 mode (mode 0, where SCK
   normally low and slave samples on SCK rising) or SPI 1,1 mode (mode
   3, where SCK normally high and slave samples on SCK rising).
   The max SPI clock frequency is 10\,MHz.  */


#define MCP6S2X_INSN_REGISTER_WRITE(REG) ((MCP6S2X_INSN_WRITE << 5) | (REG))

#define MCP6S2X_INSN_GAIN_REGISTER_WRITE MCP6S2X_INSN_REGISTER_WRITE (MCP6S2X_GAIN_REGISTER)
#define MCP6S2X_INSN_CHANNEL_REGISTER_WRITE MCP6S2X_INSN_REGISTER_WRITE (MCP6S2X_CHANNEL_REGISTER)


/* Valid MCP6S2X commands must always consist of a multiple of 16
   bits, sent while the chip is selected.  */


static const spi_pga_gain_t mcp6sx_gains[] = {1, 2, 4, 5, 8, 10, 16, 32, 0};


static bool
mcp6sx_gain_set (spi_pga_t pga, uint8_t gain_index)
{
    uint8_t command[] = {MCP6S2X_INSN_GAIN_REGISTER_WRITE, 0};

    command[1] = gain_index;

    /* This will wake up the PGA from shutdown.  */
    return spi_pga_command (pga, command, ARRAY_SIZE (command));
}


static spi_pga_channel_t
mcp6sx_channel_set (spi_pga_t pga, spi_pga_channel_t channel)
{
    uint8_t command[] = {MCP6S2X_INSN_CHANNEL_REGISTER_WRITE, channel};

    return spi_pga_command (pga, command, ARRAY_SIZE (command));
}


static bool 
mcp6sx_shutdown_set (spi_pga_t pga, bool enable)
{
    uint8_t command[] = {MCP6S2X_INSN_SHUTDOWN, 0};

    /* The only way to wake the MCP6S2X up is to send it a valid command.  */

    if (enable)
        return spi_pga_command (pga, command, ARRAY_SIZE (command));
    else
        return mcp6sx_gain_set (pga, 1);
}


spi_pga_ops_t mcp6s2x_ops =
{
    .gain_set = mcp6sx_gain_set,   
    .channel_set = mcp6sx_channel_set,   
    .offset_set = 0,
    .shutdown_set = mcp6sx_shutdown_set,   
    .gains = mcp6sx_gains
};

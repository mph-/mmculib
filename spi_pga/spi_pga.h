/** @file   spi_pga.h
    @author M. P. Hayes, UCECE
    @date   09 August 2007
    @brief  SPI PGA
*/

#ifndef SPI_PGA_H
#define SPI_PGA_H

#include "config.h"


#if defined (SPI_PGA_MCP6S21) || defined (SPI_PGA_MCP6S22)
   || defined (SPI_PGA_MCP6S26) || defined (SPI_PGA_MCP6S28)
#include "mcp6s21.h"


#define pga_gain_set mcp6s21_gain_set

#define pga_channel_set mcp6s21_channel_set

#define pga_wakeup mcp6s21_wakeup

#define pga_shutdown mcp6s21_shutdown

#define pga_init mcp6s21_init


#else
#error Undefined SPI_PGA_TYPE
#endif



#endif



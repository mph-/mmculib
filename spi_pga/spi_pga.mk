SPI_PGA_DIR = $(DRIVER_DIR)/spi_pga

VPATH += $(SPI_PGA_DIR)
SRC += spi_pga.c max9939.c mcp6s2x.c

INCLUDES += -I$(SPI_PGA_DIR)

SPI_DATAFLASH_DIR = $(DRIVER_DIR)/spi_dataflash

VPATH += $(SPI_DATAFLASH_DIR) $(ARCH_DIR)
SRC += spi_dataflash.c spi.c

INCLUDES += -I$(SPI_DATAFLASH_DIR) -I$(SPI_DATAFLASH_DIR)/../utility/

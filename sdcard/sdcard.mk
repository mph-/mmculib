SDCARD_DIR = $(DRIVER_DIR)/sdcard

VPATH += $(SDCARD_DIR) $(ARCH_DIR)
SRC += sdcard.c

PERIPHERALS += spi

INCLUDES += -I$(SDCARD_DIR) -I$(SDCARD_DIR)/../utility/
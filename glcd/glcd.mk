GLCD_DIR = $(DRIVER_DIR)/glcd

VPATH += $(GLCD_DIR)
INCLUDES += -I$(GLCD_DIR)

PERIPHERALS += spi

SRC += glcd.c glcd_text.c font.c


BUART_DIR = $(DRIVER_DIR)/buart

VPATH += $(BUART_DIR)
INCLUDES += -I$(BUART_DIR)

PERIPHERALS += uart
DRIVERS += ring

SRC += buart.c


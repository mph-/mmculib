BUSART_DIR = $(DRIVER_DIR)/busart

VPATH += $(BUSART_DIR)
INCLUDES += -I$(BUSART_DIR)

PERIPHERALS += usart
DRIVERS += ring

SRC += busart.c


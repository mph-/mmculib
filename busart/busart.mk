BUSART_DIR = $(DRIVER_DIR)/busart

VPATH += $(BUSART_DIR)
INCLUDES += -I$(BUSART_DIR) -I$(BUSART_DIR)/../utility/

PERIPHERALS += usart

SRC += busart.c ring.c


USART_SERIAL_DIR = $(DRIVER_DIR)/usart_serial

VPATH += $(USART_SERIAL_DIR)
INCLUDES += -I$(USART_SERIAL_DIR)

DRIVERS += busart tty

SRC += usart_serial.c




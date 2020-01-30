USB_SERIAL_DIR = $(DRIVER_DIR)/usb_serial

VPATH += $(USB_SERIAL_DIR)
INCLUDES += -I$(USB_SERIAL_DIR)

DRIVERS += usb_cdc tty

SRC += usb_serial.c

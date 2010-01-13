USB_DIR = $(DRIVER_DIR)/usb

VPATH += $(USB_DIR)
INCLUDES += -I$(USB_DIR)

PERIPHERALS += udp

SRC += usb.c





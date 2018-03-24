USB_CDC_DIR = $(DRIVER_DIR)/usb_cdc

VPATH += $(USB_CDC_DIR)
INCLUDES += -I$(USB_CDC_DIR)

PERIPHERALS += udp
DRIVERS += usb ring

SRC += usb_cdc.c usb.c




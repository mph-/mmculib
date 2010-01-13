USB_MSD_DIR = $(DRIVER_DIR)/usb_msd

VPATH += $(USB_MSD_DIR) $(ARCH_DIR)
INCLUDES += -I$(USB_MSD_DIR) -I$(USB_MSD_DIR)/../usb/

PERIPHERALS += udp

SRC += usb_bot.c usb_lun.c usb_msd.c usb_msd_dsc.c usb_sbc.c usb.c spi.c





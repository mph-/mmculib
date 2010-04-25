USB_MSD_DIR = $(DRIVER_DIR)/usb_msd

VPATH += $(USB_MSD_DIR) $(ARCH_DIR)
INCLUDES += -I$(USB_MSD_DIR) -I$(USB_MSD_DIR)/../usb/

PERIPHERALS += udp
DRIVERS += usb

SRC += usb_msd.c usb_msd_dsc.c usb_msd_sbc.c usb_msd_lun.c usb_bot.c usb.c





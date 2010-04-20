SDCARD_MSD_DIR = $(DRIVER_DIR)/sdcard_msd

VPATH += $(SDCARD_MSD_DIR)
INCLUDES += -I$(SDCARD_MSD_DIR)

DRIVERS += sdcard
SRC += sdcard_msd.c msd.c




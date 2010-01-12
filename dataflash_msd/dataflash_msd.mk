DATAFLASH_MSD_DIR = $(DRIVER_DIR)/dataflash_msd

VPATH += $(DATAFLASH_MSD_DIR)
INCLUDES += -I$(DATAFLASH_MSD_DIR)

SRC += dataflash_msd.c msd.c




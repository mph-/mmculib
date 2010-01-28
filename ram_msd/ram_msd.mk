RAM_MSD_DIR = $(DRIVER_DIR)/ram_msd

VPATH += $(RAM_MSD_DIR)
INCLUDES += -I$(RAM_MSD_DIR)

SRC += ram_msd.c msd.c




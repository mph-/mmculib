FAT_FS_DIR = $(DRIVER_DIR)/fat_fs

VPATH += $(FAT_FS_DIR)
INCLUDES += -I$(FAT_FS_DIR)

SRC += fat_fs.c fat.c msd.c




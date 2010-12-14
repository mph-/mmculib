FAT_DIR = $(DRIVER_DIR)/fat_fs

VPATH += $(FAT_DIR)
INCLUDES += -I$(FAT_DIR)

SRC += fat_file.c fat_de.c fat_fsinfo.c fat_cluster.c fat_partition.c fat_boot.c fat_io.c fat_debug.c fat_test.c fat_stats.c

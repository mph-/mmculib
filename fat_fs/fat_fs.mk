FAT_FS_DIR = $(DRIVER_DIR)/fat_fs

VPATH += $(FAT_FS_DIR)
INCLUDES += -I$(FAT_FS_DIR)

SRC += fat_fs.c msd.c fat_file.c fat_de.c fat_fsinfo.c fat_cluster.c fat_partition.c fat_boot.c fat_io.c fat_debug.c fat_test.c fat_stats.c





CRC_DIR = $(DRIVER_DIR)/crc

VPATH += $(CRC_DIR)
SRC += crc8541.c dscrc8.c dscrc16.c

INCLUDES += -I$(CRC_DIR)


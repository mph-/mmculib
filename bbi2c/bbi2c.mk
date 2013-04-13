BBI2C_DIR = $(DRIVER_DIR)/bbi2c

VPATH += $(BBI2C_DIR)
SRC += i2c_master.c

INCLUDES += -I$(BBI2C_DIR)


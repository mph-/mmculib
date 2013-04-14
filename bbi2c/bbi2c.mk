BBI2C_DIR = $(DRIVER_DIR)/bbi2c

VPATH += $(BBI2C_DIR)
SRC += i2c_master.c i2c_slave.c

INCLUDES += -I$(BBI2C_DIR)


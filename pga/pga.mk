PGA_DIR = $(DRIVER_DIR)/pga

VPATH += $(PGA_DIR)
SRC += pga.c

INCLUDES += -I$(PGA_DIR)

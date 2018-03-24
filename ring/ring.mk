RING_DIR = $(DRIVER_DIR)/ring

VPATH += $(RING_DIR)
INCLUDES += -I$(RING_DIR) 

SRC += ring.c

TTY_DIR = $(DRIVER_DIR)/tty

DRIVERS += ring

VPATH += $(TTY_DIR)
SRC += tty.c linebuffer.c
INCLUDES += -I$(TTY_DIR) 


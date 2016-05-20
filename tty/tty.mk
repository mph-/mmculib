TTY_DIR = $(DRIVER_DIR)/tty

VPATH += $(TTY_DIR)
SRC += tty.c linebuffer.c utility/ring.c
INCLUDES += -I$(TTY_DIR) -I$(TTY_DIR)/../utility/


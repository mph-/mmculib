CAPTURE_DIR = $(MMCULIB_DIR)/capture

VPATH += $(CAPTURE_DIR)
INCLUDES += -I$(CAPTURE_DIR)

SRC += capture.c


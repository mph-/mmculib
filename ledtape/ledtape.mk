LEDTAPE_DIR = $(DRIVER_DIR)/ledtape

VPATH += $(LEDTAPE_DIR)
SRC += ledtape.c ledbuffer.c

INCLUDES += -I$(LEDTAPE_DIR)
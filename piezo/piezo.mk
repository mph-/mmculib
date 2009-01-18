PIEZO_DIR = $(DRIVER_DIR)/piezo

VPATH += $(PIEZO_DIR)
SRC += piezo.c piezo_beep.c

INCLUDES += -I$(PIEZO_DIR)

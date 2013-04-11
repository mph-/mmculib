GSM_DIR = $(DRIVER_DIR)/gsm

VPATH += $(GSM_DIR)
INCLUDES += -I$(GSM_DIR)

DRIVERS += busart
SRC += gsm862.c





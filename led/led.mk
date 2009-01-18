LED_DIR = $(DRIVER_DIR)/led

VPATH += $(LED_DIR)
SRC += led.c

INCLUDES += -I$(LED_DIR)


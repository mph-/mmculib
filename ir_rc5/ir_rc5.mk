IR_RC5_DIR = $(DRIVER_DIR)/ir_rc5

VPATH += $(IR_RC5_DIR)
SRC += ir_rc5_rx.c

INCLUDES += -I$(IR_RC5_DIR)


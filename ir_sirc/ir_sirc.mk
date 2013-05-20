IR_SIRC_DIR = $(DRIVER_DIR)/ir_sirc

VPATH += $(IR_SIRC_DIR)
SRC += ir_sirc_rx.c

INCLUDES += -I$(IR_SIRC_DIR)


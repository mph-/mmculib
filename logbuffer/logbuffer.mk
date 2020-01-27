SYSLOG_DIR = $(DRIVER_DIR)/logbuffer

VPATH += $(SYSLOG_DIR)
INCLUDES += -I$(SYSLOG_DIR)

DRIVERS += ring

SRC += logbuffer.c

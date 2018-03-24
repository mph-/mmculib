SYSLOG_DIR = $(DRIVER_DIR)/syslog

VPATH += $(SYSLOG_DIR)
INCLUDES += -I$(SYSLOG_DIR)

DRIVERS += ring

SRC += syslog.c

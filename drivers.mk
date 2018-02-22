DRIVER_DIR = $(MMCULIB_DIR)

include $(foreach driver, $(DRIVERS), $(DRIVER_DIR)/$(driver)/$(driver).mk)

# Perform second pass for drivers that depend on other drivers
include $(foreach driver, $(DRIVERS), $(DRIVER_DIR)/$(driver)/$(driver).mk)

VPATH += $(DRIVER_DIR)/utility $(DRIVER_DIR)

INCLUDES += -I$(DRIVER_DIR)


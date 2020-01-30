DRIVER_DIR = $(MMCULIB_DIR)


include $(foreach driver, $(DRIVERS), $(DRIVER_DIR)/$(driver)/$(driver).mk)
# Remove duplicates
DRIVERS := $(sort $(DRIVERS))

# Ideally should iterate until no new drivers added...

# Perform second pass for drivers that depend on other drivers
include $(foreach driver, $(DRIVERS), $(DRIVER_DIR)/$(driver)/$(driver).mk)
DRIVERS := $(sort $(DRIVERS))

# Perform third pass for drivers that depend on other drivers
include $(foreach driver, $(DRIVERS), $(DRIVER_DIR)/$(driver)/$(driver).mk)
DRIVERS := $(sort $(DRIVERS))

VPATH += $(DRIVER_DIR)/utility $(DRIVER_DIR)

INCLUDES += -I$(DRIVER_DIR)


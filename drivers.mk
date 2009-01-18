include $(foreach driver, $(DRIVERS), $(DRIVER_DIR)/$(driver)/$(driver).mk)

VPATH += $(DRIVER_DIR)/utility $(DRIVER_DIR)

INCLUDES += -I$(DRIVER_DIR)


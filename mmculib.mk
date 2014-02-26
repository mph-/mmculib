# Need to define:
# MCU name of microcontroller
# DRIVERS list of drivers to build
# PERIPHERALS list of peripherals to build
# SRC list of additional C source files

ifndef MCU
$(error MCU undefined, this needs to be defined in the Makefile)
endif


# Include the architecture independent driver dependencies.
include $(MMCULIB_DIR)/drivers.mk


# Include the architecture dependent dependencies and peripheral support.
ifneq (, $(findstring SAM, $(MCU)))
# AT91SAM7 family
ifndef MAT91LIB_DIR
MAT91LIB_DIR = $(MMCULIB_DIR)/../mat91lib
endif
include $(MAT91LIB_DIR)/mat91lib.mk
else ifneq (, $(findstring ATmega, $(MCU)))
# AVR family
ifndef MAVRLIB_DIR
MAVRLIB_DIR = $(MMCULIB_DIR)/../mavrlib
endif
include $(MAVRLIB_DIR)/mavrlib.mk
else ifneq (, $(findstring BF5, $(MCU)))
# Blackfin family
ifndef MBFLIB_DIR
MBFLIB_DIR = $(MMCULIB_DIR)/../mbflib
endif
include $(MBFLIB_DIR)/mbflib.mk
endif



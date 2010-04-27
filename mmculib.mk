# Need to define:
# ROOT toplevel directory of project
# OPT optimisation level, e.g. -O2
# MCU name of microcontroller
# RUN_MODE either ROM_RUN or RAM_RUN
# MMCULIB_DIR path to mmculib
# TARGET name of target to build, e.g., treetap.bin
# DRIVERS list of drivers to build
# PERIPHERALS list of peripherals to build
# SRC list of additional C source files

ifndef ROOT
ROOT = .
endif

ifndef MCU
MCU = AT91SAM7S256
endif

INCLUDES += -I.

ifdef BOARD
CFLAGS += -D$(BOARD)
endif

# Create list of object and dependency files.  Note, sort removes duplicates.
OBJ = $(addprefix objs/, $(sort $(SRC:.c=.o)))
DEPS = $(addprefix deps/, $(sort $(SRC:.c=.d)))

TARGET_OUT = $(TARGET:.bin=.out)
TARGET_MAP = $(TARGET:.bin=.map)

.SUFFIXES:


all: $(TARGET)

objs:
	mkdir -p objs

deps:
	mkdir -p deps

objs/%.o: %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

# Automatically generate C source code dependencies.
deps/%.d: %.c deps
	set -e; $(CC) -MM $(CFLAGS) $< \
	| sed 's,\(.*\)\.o[ :]*,objs/\1.o deps/\1.d : ,g' > $@; \
	[ -s $@ ] || rm -f $@

# Include the driver dependencies.
include $(MMCULIB_DIR)/drivers.mk

# Include the architecture dependent dependencies.
ifneq (, $(findstring AT91, $(MCU)))
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


# Include the dependency files.
-include $(DEPS)


# Link object files to form output file.
$(TARGET_OUT): objs $(OBJ) $(EXTRA_OBJ)
	$(CC) $(OBJ) $(EXTRA_OBJ) $(LDFLAGS) -o $@ -lm -Wl,-Map=$(TARGET_MAP),--cref
	$(SIZE) $@

# Convert output file to binary image file.
%.bin: %.out
	$(OBJCOPY) --output-target=binary $^ $@

# Remove the objs directory
clean-objs:
	-$(DEL) -fr objs

# Rebuild the code, don't delete dependencies.
rebuild: clean-objs $(TARGET_OUT)

# Remove non-source files.
.PHONY: clean
clean: 
	-$(DEL) -f *.o *.out *.hex *.bin *.elf *.d *.lst *.map *.sym *.lss *.cfg *.ocd *~
	-$(DEL) -fr objs deps




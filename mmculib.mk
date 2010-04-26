# Need to define:
# ROOT toplevel directory of project
# OPT optimisation level, e.g. -O2
# MCU name of microcontroller
# RUN_MODE either ROM_RUN or RAM_RUN
# MAT91LIB_DIR path to mat91lib
# MMCULIB_DIR path to mmculib
# TARGET name of target to build, e.g., treetap.bin
# DRIVERS list of drivers to build
# PERIPHERALS list of peripherals to build
# SRC list of additional C source files


ifndef ROOT
ROOT = .
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

include $(MMCULIB_DIR)/drivers.mk
include $(MAT91LIB_DIR)/mat91lib.mk

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




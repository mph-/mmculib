CFLAGS = -g -I.. -I.

DRIVER_DIR = ../..

include ../fat.mk

OBJ = $(SRC:.c=.o)

all: fattest1 fatdump

fattest1: fattest1.o $(OBJ)

fatdump: fatdump.o $(OBJ)

fs.fat:
	mkfs.dos -C 8192 $@

clean:
	rm *.o


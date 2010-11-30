CFLAGS = -g -I.. -I.

DRIVER_DIR = ../..
SRC = fattest1.c

include ../fat.mk

OBJ = $(SRC:.c=.o)

all: fattest1

fattest1: $(OBJ)

clean:
	rm *.o


CC = gcc
CFLAGS = -g -I.. -I.

VPATH = ..

all: chaser-test uint8toa-test muxleds-test squeaker-test ring-test

chaser-test: chaser-test.o chaser.o flasher.o font.o

uint8toa-test: uint8toa-test.o uint8toa.o

muxleds-test: muxleds-test.o muxleds.o

squeaker-test: squeaker-test.o squeaker.o seq.o

ring-test: ring-test.o ring.o

include ../flags.mk

CPPFLAGS += \
	-D_POSIX_C_SOURCE=200809L \

CFLAGS += \

.PHONY: all
all: square_evictions

rng_test: rng.o
square_evictions: llc.o

llc.o: llc.h
rng.o: rng.h

include ../flags.mk

CPPFLAGS += \
	-D_BSD_SOURCE \
	-D_POSIX_C_SOURCE=200809L \

CFLAGS += \

.PHONY: all
all: square_evictions dist_a

include ../external/modules.mk

rng_test: rng.o
square_evictions: llc.o rng.o
dist_a: ../external/pqos/lib/libpqos.a dist_a.c llc.o rng.o

llc.o: llc.h
rng.o: rng.h

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S $^ -o $@

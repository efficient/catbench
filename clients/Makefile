include ../flags.mk

CPPFLAGS += \
	-D_DEFAULT_SOURCE \
	-D_POSIX_C_SOURCE=200809L \
	-D_GNU_SOURCE \

CFLAGS += \

.PHONY: all
all: square_evictions

include ../external/modules.mk

rng_test: rng.o
square_evictions: llc.o movnt.o perf_poll.o rng.o

llc.o: llc.h
movnt.o: movnt.h
perf_poll.o: perf_poll.h syscallers.h
rng.o: rng.h

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S $^ -o $@

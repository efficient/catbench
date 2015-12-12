include ../flags.mk

CPPFLAGS += \
	-D_BSD_SOURCE \
	-D_GNU_SOURCE \
	-D_POSIX_C_SOURCE=200809L \

CFLAGS += \

.PHONY: all
all: square_evictions dist_a

include ../external/modules.mk

rng_test: rng.o
square_evictions: llc.o movnt.o perf_poll.o rng.o

llc.o: llc.h
movnt.o: movnt.h
perf_poll.o: perf_poll.h syscallers.h
rng.o: rng.h

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S $^ -o $@

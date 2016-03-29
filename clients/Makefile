include ../flags.mk

CPPFLAGS += \
	-D_DEFAULT_SOURCE \
	-D_POSIX_C_SOURCE=200809L \
	-D_GNU_SOURCE \

CFLAGS += \

.PHONY: all
all: square_evictions

include ../external/modules.mk

basicfwd: CFLAGS += -std=gnu99 -Wno-inline
basicfwd: CPPFLAGS += -include rte_config.h
basicfwd: LDLIBS += -ldl -lpthread
basicfwd: ../external/dpdk/build/lib/libethdev.a ../external/dpdk/build/lib/librte_mbuf.a ../external/dpdk/build/lib/librte_eal.a ../external/dpdk/build/lib/librte_mempool.a ../external/dpdk/build/lib/librte_ring.a
rng_test: rng.o
square_evictions: llc.o movnt.o perf_poll.o rng.o

llc.o: llc.h
movnt.o: movnt.h
perf_poll.o: perf_poll.h syscallers.h
rng.o: rng.h

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S $^ -o $@

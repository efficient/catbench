include ../flags.mk

CPPFLAGS += \
	-D_DEFAULT_SOURCE \
	-D_POSIX_C_SOURCE=200809L \
	-D_GNU_SOURCE \

CFLAGS += \

# Add to LDLIBS of targets needing to use the DPDK driver.
LINK_PMD_IXGBE := -Wl,--whole-archive ../external/dpdk/build/lib/librte_pmd_ixgbe.a -Wl,--no-whole-archive

.PHONY: all
all: square_evictions

include ../external/modules.mk

basicfwd: CFLAGS += -std=gnu99 -Wno-inline
basicfwd: CPPFLAGS += -include rte_config.h
basicfwd: LDLIBS += $(LINK_PMD_IXGBE) -ldl -lm -lpthread -lrt
basicfwd: ../external/dpdk/build/lib/libethdev.a ../external/dpdk/build/lib/librte_mbuf.a ../external/dpdk/build/lib/librte_eal.a ../external/dpdk/build/lib/librte_mempool.a ../external/dpdk/build/lib/librte_ring.a
lockserver-ng: LDLIBS += $(LINK_PMD_IXGBE) -ldl -lpthread
lockserver-ng: lockserver-ng.o dpdk_wrapper.o ../external/dpdk/build/lib/libethdev.a ../external/dpdk/build/lib/librte_mbuf.a ../external/dpdk/build/lib/librte_eal.a ../external/dpdk/build/lib/librte_mempool.a ../external/dpdk/build/lib/librte_ring.a
rng_test: rng.o
square_evictions: llc.o movnt.o perf_poll.o rng.o

dpdk_wrapper.%: CFLAGS += -std=gnu99 -Wno-cast-qual
dpdk_wrapper.%: CPPFLAGS += -include rte_config.h
dpdk_wrapper.%: ../external/dpdk/build/lib/libethdev.a
dpdk_wrapper.%: dpdk_wrapper.h
llc.o: llc.h
lockserver-ng.%: CFLAGS += -std=gnu99
lockserver-ng.%: CPPFLAGS += -include rte_config.h
lockserver-ng.%: ../external/dpdk/build/lib/libethdev.a
lockserver-ng.%: dpdk_wrapper.h
movnt.o: movnt.h
perf_poll.o: perf_poll.h syscallers.h
rng.o: rng.h

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S $^ -o $@

include ../flags.mk

CPPFLAGS += \
	-D_DEFAULT_SOURCE \
	-D_POSIX_C_SOURCE=200809L \
	-D_GNU_SOURCE \

CFLAGS += \

LDLIBS += \
	-lrt \

# Add to LDLIBS of targets needing to use the DPDK driver.
LINK_PMD_IXGBE := -Wl,--whole-archive ../external/dpdk/build/lib/librte_pmd_ixgbe.a -Wl,--no-whole-archive

.PHONY: all
all: square_evictions lockserver-ng lockclient

include ../external/modules.mk

lockclient: LDLIBS += ../external/dpdk/build/lib/libethdev.a ../external/dpdk/build/lib/librte_mbuf.a ../external/dpdk/build/lib/librte_eal.a ../external/dpdk/build/lib/librte_mempool.a ../external/dpdk/build/lib/librte_ring.a
lockclient: LDLIBS += $(LINK_PMD_IXGBE) -ldl -lpthread
lockclient: lockclient.o dpdk_wrapper.o realtime.o
lockserver-ng: LDLIBS += ../external/dpdk/build/lib/libethdev.a ../external/dpdk/build/lib/librte_mbuf.a ../external/dpdk/build/lib/librte_eal.a ../external/dpdk/build/lib/librte_mempool.a ../external/dpdk/build/lib/librte_ring.a
lockserver-ng: LDLIBS += $(LINK_PMD_IXGBE) -ldl -lpthread
lockserver-ng: lockserver-ng.o dpdk_wrapper.o realtime.o
rng_test: rng.o
square_evictions: llc.o movnt.o perf_poll.o realtime.o rng.o

dpdk_wrapper.o: CFLAGS += -std=gnu99 -Wno-cast-qual
dpdk_wrapper.o: CPPFLAGS += -include rte_config.h
dpdk_wrapper.o: ../external/dpdk/build/lib/libethdev.a
dpdk_wrapper.o: dpdk_wrapper.h
llc.o: llc.h
lockclient.o: CFLAGS += -std=gnu99
lockclient.o: CPPFLAGS += -include rte_config.h
lockclient.o: ../external/dpdk/build/lib/libethdev.a
lockclient.o: dpdk_wrapper.h realtime.h
lockserver-ng.o: CFLAGS += -std=gnu99
lockserver-ng.o: CPPFLAGS += -include rte_config.h
lockserver-ng.o: ../external/dpdk/build/lib/libethdev.a
lockserver-ng.o: dpdk_wrapper.h realtime.h
movnt.o: movnt.h
perf_poll.o: perf_poll.h syscallers.h
rng.o: rng.h
square_evictions.o: movnt.h perf_poll.h realtime.h rng.h

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S $^ -o $@

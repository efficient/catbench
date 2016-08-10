include flags.mk

CPPFLAGS += \
	-DLOG_DEFAULT_VERBOSITY=LOG_VERBOSE \
	-D_GNU_SOURCE \

CFLAGS += \

.PHONY: all
all: catbench-setcap dist_a slowbro locksmith

.PHONY: clean
clean:
	git clean -fX $(wildcard *)

.PHONY: clean-recursive
clean-recursive: clean
	$(RM) -r external/dpdk/build
	$(RM) external/pqos/lib/libpqos.a.dep external/pqos/lib/libpqos.so.1 external/pqos/lib/libpqos.so.1.0.1 external/pqos/pqos
	git submodule foreach '[ -f "../`basename $$PWD`.patch" ] && git apply -R "../`basename $$PWD`.patch"; true'
	git submodule foreach git clean -fX
	git submodule deinit $(DEINIT_FLAGS) .

include external/modules.mk

.PHONY: catbench-setcap

catbench-setcap: catbench
	sudo setcap cap_sys_nice+ep $<

# external/pqos/lib/libpqos.a should be at the end of these lines to compile correctly
catbench: log.o proc_manip.o syscallers.h prep_system.o bench_commands.o external/pqos/lib/libpqos.a
dist_a: rdtscp.h log.o proc_manip.o prep_system.o bench_commands.o external/pqos/lib/libpqos.a
slowbro: bench_commands.o log.o prep_system.o proc_manip.o external/pqos/lib/libpqos.a
locksmith: bench_commands.o log.o prep_system.o proc_manip.o external/pqos/lib/libpqos.a

perfexpl: syscallers.h

bench_commands.o: bench_commands.h syscallers.h
log.o: log.h
prep_system.o: external/pqos/lib/libpqos.a
proc_manip.o: proc_manip.h

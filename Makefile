include flags.mk

CPPFLAGS += \
	-DLOG_DEFAULT_VERBOSITY=LOG_VERBOSE \
	-D_GNU_SOURCE \

CFLAGS += \

.PHONY: all
all: catbench-setcap

.PHONY: clean
clean:
	git clean -fX

.PHONY: clean-recursive
clean-recursive: clean
	$(RM) external/pqos/lib/libpqos.a.dep external/pqos/lib/libpqos.so.1 external/pqos/lib/libpqos.so.1.0.1 external/pqos/pqos
	git submodule foreach '[ -f "../`basename $$PWD`.patch" ] && git apply -R "../`basename $$PWD`.patch"; true'
	git submodule foreach git clean -fX
	git submodule deinit .

include external/modules.mk

.PHONY: catbench-setcap
catbench-setcap: catbench
	sudo setcap cap_sys_nice+ep $<

catbench: log.o proc_manip.o syscallers.h external/pqos/lib/libpqos.a prep_system.o bench_commands.o

perfexpl: syscallers.h

log.o: log.h
proc_manip.o: proc_manip.h

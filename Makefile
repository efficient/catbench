include flags.mk

CPPFLAGS += \
	-DLOG_DEFAULT_VERBOSITY=LOG_VERBOSE \
	-D_GNU_SOURCE \

CFLAGS += \
	-pthread \

.PHONY: all
all: catbench-setcap

.PHONY: clean
clean:
	git clean -fX

.PHONY: clean-recursive
clean-recursive: clean
	$(RM) external/pqos/lib/libpqos.a.dep external/pqos/lib/libpqos.so.1 external/pqos/lib/libpqos.so.1.0.1
	git submodule foreach git clean -fX
	git submodule deinit .

include external/modules.mk

external/pqos/lib/libpqos.so: MAKE += SHARED=y
external/pqos/lib/libpqos.so: external/pqos/lib/libpqos.so.1
	[ -e $@ ] || ln -s $(<F) $@

.PHONY: catbench-setcap
catbench-setcap: catbench
	sudo setcap cap_sys_nice+ep $<

catbench: LDLIBS += -lpqos
catbench: cpu_support.o cpuid.o log.o proc_manip.o | external/pqos/lib/libpqos.so

cpu_support.o: cpu_support.h
cpuid.o: cpuid.h
log.o: log.h
proc_manip.o: proc_manip.h

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

.PHONY: catbench-setcap
catbench-setcap: catbench
	sudo setcap cap_sys_nice+ep $<

catbench: cpu_support.o cpuid.o log.o proc_manip.o

cpu_support.o: cpu_support.h
cpuid.o: cpuid.h
log.o: log.h
proc_manip.o: proc_manip.h

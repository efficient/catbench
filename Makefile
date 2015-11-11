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

.PHONY: catbench-setcap
catbench-setcap: catbench
	sudo setcap cap_sys_nice+ep $<

catbench: cpu_support.o cpuid.o log.o proc_manip.o

CPPFLAGS := \
	-DLOG_DEFAULT_VERBOSITY=LOG_VERBOSE \
	-D_GNU_SOURCE \
	-Wundef \

CFLAGS := \
	-Og \
	-g3 \
	-std=c99 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wcast-qual \
	-Wfloat-equal \
	-Winline \
	-Wmissing-include-dirs \
	-Wmissing-prototypes \
	-Wredundant-decls \
	-Wstrict-prototypes \
	-Wwrite-strings \
	-Wno-unused-function \

.PHONY: all
all: catbench-setcap

.PHONY: clean
clean:
	git clean -fX

.PHONY: catbench-setcap
catbench-setcap: catbench
	sudo setcap cap_sys_nice+ep $<

catbench: cpu_support.o cpuid.o log.o proc_manip.o

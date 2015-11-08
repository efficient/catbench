CPPFLAGS := \
	-DLOG_DEFAULT_VERBOSITY=LOG_VERBOSE \
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
all: catbench

.PHONY: clean
clean:
	git clean -fX

catbench: cpu_support.o cpuid.o log.o

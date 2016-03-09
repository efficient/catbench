CPPFLAGS_global := \
	-Wundef \
	-isystem $(dir $(lastword $(MAKEFILE_LIST)))external/dpdk/build/include \
	-isystem $(dir $(lastword $(MAKEFILE_LIST)))external/pqos/lib \

CFLAGS_global := \
	-Og \
	-g3 \
	-std=c99 \
	-Wall \
	-Wextra \
	-Wcast-qual \
	-Wfloat-equal \
	-Winline \
	-Wmissing-prototypes \
	-Wredundant-decls \
	-Wstrict-prototypes \
	-Wwrite-strings \
	-Wno-unused-function \
	-Wpedantic \

LDFLAGS_global := \
	-L$(dir $(lastword $(MAKEFILE_LIST)))external/dpdk/build/lib \
	-L$(dir $(lastword $(MAKEFILE_LIST)))external/pqos/lib \

CPPFLAGS += $(CPPFLAGS_global)
CFLAGS += $(CFLAGS_global)
LDFLAGS += $(LDFLAGS_global)

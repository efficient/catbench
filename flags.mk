CPPFLAGS += \
	-Wundef \
	-isystem external/pqos/lib \

CFLAGS += \
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
#	Breaks pqos.h
#	-Wpedantic \

LDFLAGS += \
	-Lexternal/pqos/lib \

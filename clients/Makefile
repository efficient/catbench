include ../flags.mk

CPPFLAGS += \
	-D_DEFAULT_SOURCE \
	-D_POSIX_C_SOURCE=200809L \
	-D_GNU_SOURCE \

CFLAGS += \

.PHONY: all
all: square_evictions

include ../external/modules.mk

rng_test: rng.o
square_evictions: square_evictions.o llc.o movnt.o perf_poll.o rng.o

llc.o: llc.h
movnt.o: movnt.h
perf_poll.o: perf_poll.h syscallers.h
rng.o: rng.h
square_evictions.o: square_evictions.s
	$(AS) $(ASFLAGS) $^ -o $@

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S $^ -o $@
	mv $@ $@.tmp
	# Replace all line breaks with carriage returns to make processing easier
	paste -sd'\r' $@.tmp >$@
	rm $@.tmp
	# Hoist sum zero-initialization into switch "preheader"
	sed -i 's/\(movl	%eax, %e\)ax\(\r[[:space:]]*jmp	\*.L[[:digit:]]\+(,%r\)a\(x,8)\)/\1cx\r	xorq	%rax, %rax\2c\3/' $@
	sed -i 's/.LVL[[:digit:]]\+:\r[[:space:]]*jmp	.L[[:digit:]]\+\r\([[:space:]]*.LVL[[:digit:]]\+:\r[[:space:]]*.L[[:digit:]]\+:\)\r[[:space:]]*.loc[^\r]*\r[[:space:]]*movl[^\r]*\r\([[:space:]]*.LVL[[:digit:]]\+:\)\r[[:space:]]*.L[[:digit:]]\+:/\1\r\2/g' $@
	# Put the line breaks back!
	sed -i 's/\r/\n/g' $@

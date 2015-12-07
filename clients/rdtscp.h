#ifndef RDTSCP_H_
#define RDTSCP_H_

#include <stdint.h>

inline uint64_t rdtscp(void) {
	uint32_t cycles_low, cycles_high;
	__asm volatile(
		"RDTSCP\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t"
		: "=r" (cycles_high), "=r" (cycles_low)
		);
	return ((uint64_t)cycles_high << 32 | cycles_low);
}
#endif

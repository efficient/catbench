#ifndef CATBENCH_CPUID_H_
#define CATBENCH_CPUID_H_

#include <stdint.h>

typedef struct {
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
} cpuid_t;

void cpuid(cpuid_t *buf, uint32_t leaf, uint32_t subleaf);

#endif

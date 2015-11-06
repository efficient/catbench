#ifndef CATBENCH_CPU_SUPPORT_H_
#define CATBENCH_CPU_SUPPORT_H_

#include <stdbool.h>
#include <stdint.h>

#include "log.h"

typedef struct {
	int cache_level;
	bool supported;
	uint8_t num_ways;
	unsigned shared_ways_mask;
	uint16_t num_classes;
} cpu_support_cat_t;

typedef struct {
	int num_cores;
	bool hthreaded;
	int num_cat_levels;
	cpu_support_cat_t *cat_levels;
} cpu_support_t;

void cpu_support(cpu_support_t *buf);
void cpu_support_cleanup(cpu_support_t *victim);

void cpu_support_log(log_verbosity_t verb, const cpu_support_t *feats);

#define cpu_support_foreach_cat_level(supp_cat, supp) \
	for(const cpu_support_cat_t *supp_cat = (supp).cat_levels; \
			supp_cat < (supp).cat_levels + (supp).num_cat_levels; ++supp_cat)

#endif

#include "llc.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define WAVE_CYCLES        2
#define PASSES_PER_CYCLE   100000
#define PERCENT_CONTRACTED 80
#define PERCENT_EXPANDED   220

int main(void) {
	llc_init();
	const int cache_line_size = llc_line_size();
	const int cache_num_sets = llc_num_sets();
	llc_cleanup();
	if(cache_line_size <= 0 || cache_num_sets <= 0)
		return 1;

	const int CAPACITY_CONTRACTED =
			cache_line_size * cache_num_sets * PERCENT_CONTRACTED / 100.0;
	const int CAPACITY_EXPANDED =
			cache_line_size * cache_num_sets * PERCENT_EXPANDED / 100.0;

	uint8_t *large = malloc(CAPACITY_EXPANDED);
	if(!large) { // "at large"
		perror("Allocating large array");
		return 1;
	}
		
	for(int cycle = 0; cycle < 2 * WAVE_CYCLES; ++cycle) {
		const char *desc = "";
		int siz;
		uint8_t val = rand();

		if(cycle % 2) {
			siz = CAPACITY_EXPANDED;
			desc = "saturated";
		} else {
			siz = CAPACITY_CONTRACTED;
			desc = "unsaturated";
		}
		printf("Beginning %s passes\n", desc);

		for(int pass = 0; pass < PASSES_PER_CYCLE; ++pass)
			for(int offset = 0; offset < siz; offset += cache_line_size) {
				large[offset] ^= val;
				val ^= large[offset];
				large[offset] ^= val;
			}
	}

	free(large);
	return 0;
}

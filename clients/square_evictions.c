#include "llc.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define DEFAULT_NUM_PERIODS        2
#define DEFAULT_PASSES_PER_CYCLE   100000
#define DEFAULT_PERCENT_CONTRACTED 80
#define DEFAULT_PERCENT_EXPANDED   220

static bool parse_arg_arg(char flag, int *dest) {
	int val;

	if(sscanf(optarg, "%d", &val) != 1) {
		fprintf(stderr, "%c: Unexpected subargument '%s'\n", flag, optarg);
		return false;
	} else if(val < 0) {
		fprintf(stderr, "%c: Expected nonnegative integer but got '%d'\n", flag, val);
		return false;
	}

	*dest = val;
	return true;
}

static int square_evictions(int cache_line_size, int num_periods, int passes_per_cycle,
		int capacity_contracted, int capacity_expanded) {
	uint8_t *large = malloc(capacity_expanded);
	if(!large) { // "at large"
		perror("Allocating large array");
		return 1;
	}
		
	for(int cycle = 0; cycle < 2 * num_periods; ++cycle) {
		const char *desc = "";
		int siz;
		uint8_t val = rand();

		if(cycle % 2) {
			siz = capacity_expanded;
			desc = "saturated";
		} else {
			siz = capacity_contracted;
			desc = "unsaturated";
		}
		printf("Beginning %s passes\n", desc);

		for(int pass = 0; pass < passes_per_cycle; ++pass)
			for(int offset = 0; offset < siz; offset += cache_line_size) {
				large[offset] ^= val;
				val ^= large[offset];
				large[offset] ^= val;
			}
	}

	free(large);
	return 0;
}

int main(int argc, char *argv[]) {
	int num_periods = DEFAULT_NUM_PERIODS;
	int passes_per_cycle = DEFAULT_PASSES_PER_CYCLE;
	int percent_contracted = DEFAULT_PERCENT_CONTRACTED;
	int percent_expanded = DEFAULT_PERCENT_EXPANDED;

	char *invoc = argv[0];
	int each_arg;
	opterr = 0;
	while((each_arg = getopt(argc, argv, "n:p:c:e:")) != -1) {
		switch(each_arg) {
		case 'n':
			if(!parse_arg_arg(each_arg, &num_periods))
				return 1;
			break;
		case 'p':
			if(!parse_arg_arg(each_arg, &passes_per_cycle))
				return 1;
			break;
		case 'c':
			if(!parse_arg_arg(each_arg, &percent_contracted))
				return 1;
			break;
		case 'e':
			if(!parse_arg_arg(each_arg, &percent_expanded))
				return 1;
			break;
		default:
			printf("USAGE: %s [-n #] [-p #] [-c %%] [-e %%]\n", invoc);
			printf(
					" -n #: Number of PERIODS (default %d)\n"
					" -p #: Number of PASSES per period (default %d)\n"
					" -c %%: Percent cache CONTRACTED 1/2-period (default %d)\n"
					" -e %%: Percent cache EXPANDED 1/2-period (default %d)\n",
					DEFAULT_NUM_PERIODS, DEFAULT_PASSES_PER_CYCLE,
					DEFAULT_PERCENT_CONTRACTED, DEFAULT_PERCENT_EXPANDED);
			return 1;
		}
	}
	if (percent_contracted > percent_expanded) {
		fprintf(stderr, "Percent contracted (%d) is larger than percent expanded (%d)\n",
				percent_contracted, percent_expanded);
		return 1;
	}

	llc_init();
	int cache_line_size = llc_line_size();
	int cache_num_sets = llc_num_sets();
	llc_cleanup();
	if(cache_line_size <= 0 || cache_num_sets <= 0)
		return 1;

	int cap_contracted = cache_line_size * cache_num_sets * percent_contracted / 100.0;
	int cap_expanded = cache_line_size * cache_num_sets * percent_expanded / 100.0;
	return square_evictions(cache_line_size, num_periods, passes_per_cycle,
			cap_contracted, cap_expanded);
}

#include "llc.h"

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "rng.h"

#define DEFAULT_NUM_PERIODS        2
#define DEFAULT_PASSES_PER_CYCLE   100000
#define DEFAULT_PERCENT_CONTRACTED 80
#define DEFAULT_PERCENT_EXPANDED   220

#define MINIMUM_GENERATOR_PERIOD   8

static inline bool pow_of_two(int num) {
	return num && !(num & (num - 1));
}

static inline void *alloc(size_t size, bool contig) {
	if(!contig)
		return malloc(size);

	uint8_t *loc = mmap(0, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE, -1, 0);
	return loc == MAP_FAILED ? NULL : loc;
}

static inline void dealloc(size_t size, void *victim, bool contig) {
	if(contig)
		munmap(victim, size);
	else
		free(victim);
}

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
		int capacity_contracted, int capacity_expanded, bool huge, rng_t *randomize) {
	uint8_t *large = alloc(capacity_expanded, huge);
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

		clock_t start = clock();

		for(int pass = 0; pass < passes_per_cycle; ++pass) {
			bool seen_initial = false;
			for(int offset = 0; offset < siz; offset += cache_line_size) {
				unsigned ix;
				do {
					ix = randomize ? rng_lcfc(randomize) * cache_line_size :
							(unsigned) offset;
				} while(ix >= (unsigned) siz);
				if(!ix)
					seen_initial = true;

				large[ix] ^= val;
				val ^= large[ix];
				large[ix] ^= val;
			}
			assert(seen_initial);
		}

		clock_t duration = clock() - start;
		printf("Completed iteration in %.6f seconds\n", ((double) duration) / CLOCKS_PER_SEC);
	}

	dealloc(capacity_expanded, large, huge);
	return 0;
}

int main(int argc, char *argv[]) {
	int num_periods = DEFAULT_NUM_PERIODS;
	int passes_per_cycle = DEFAULT_PASSES_PER_CYCLE;
	int percent_contracted = DEFAULT_PERCENT_CONTRACTED;
	int percent_expanded = DEFAULT_PERCENT_EXPANDED;
	int custom_increment = 0;
	bool hugepages = false;
	bool randomize = false;

	char *invoc = argv[0];
	int each_arg;
	opterr = 0;
	while((each_arg = getopt(argc, argv, "n:p:c:e:hr::")) != -1) {
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
		case 'h':
			hugepages = true;
			break;
		case 'r':
			randomize = true;
			if(optarg) {
				if(!parse_arg_arg(each_arg, &custom_increment))
					return 1;
			}
			break;
		default:
			printf("USAGE: %s [-n #] [-p #] [-c %%] [-e %%] [-r [#]]\n", invoc);
			printf(
					" -n #: Number of PERIODS (default %d)\n"
					" -p #: Number of PASSES per period (default %d)\n"
					" -c %%: Percent cache CONTRACTED 1/2-period (default %d)\n"
					" -e %%: Percent cache EXPANDED 1/2-period (default %d)\n"
					" -h: Allocate using huge pages for contiguous memory\n"
					" -r [#]: Randomize ordering to fool prefetcher\n"
					"         Optionally takes a custom rng increment\n",
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

	int lines_contracted = cache_num_sets * percent_contracted / 100.0;
	int lines_expanded = cache_num_sets * percent_expanded / 100.0;

	rng_t *random = NULL;
	const int TWO = 2;
	if(randomize) {
		if(!pow_of_two(cache_num_sets)) {
			fprintf(stderr, "Number of cache sets (%d) is not a power of two\n",
					cache_num_sets);
			return 1;
		}

		unsigned siz = cache_num_sets;
		while(siz >> 1 >= (unsigned) lines_expanded && siz >= MINIMUM_GENERATOR_PERIOD)
			siz >>= 1;
		while(siz << 1 <= (unsigned) lines_expanded)
			siz <<= 1;

		if(custom_increment) {
			random = rng_lcfc_init_incr(siz, 1, &TWO, custom_increment);
			if(!random) {
				fprintf(stderr, "Provided rng increment (%d) not coprime with %d\n",
						custom_increment, cache_num_sets);
				return 1;
			}
		} else {
			random = rng_lcfc_init(siz, 1, &TWO);
			if(!random) {
				perror("Allocating random number generator");
				return 1;
			}
		}
	}

	int cap_contracted = cache_line_size * lines_contracted;
	int cap_expanded = cache_line_size * lines_expanded;
	int res = square_evictions(cache_line_size, num_periods, passes_per_cycle,
			cap_contracted, cap_expanded, hugepages, random);
	if(random)
		rng_lcfc_clean(random);
	return res;
}

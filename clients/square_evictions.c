#include "llc.h"

#include <sys/mman.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "perf_poll.h"
#include "rng.h"

#define DEFAULT_NUM_PERIODS        2
#define DEFAULT_PASSES_PER_CYCLE   100000
#define DEFAULT_PERCENT_CONTRACTED 80
#define DEFAULT_PERCENT_EXPANDED   220

#define PERF_LOG_FILE_HEADER_LINE  "realtime,cputime,instructions,bandwidth\n"
static const struct perf_event_attr PERF_LOG_COUNTERS[] = {
	{
		.type = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_INSTRUCTIONS,
		.read_format = PERF_FORMAT_GROUP,
		.disabled = 0x1,
		.size = sizeof *PERF_LOG_COUNTERS,
	},
	{
		.type = 0x8,
		.config = 0x3,
		.size = sizeof *PERF_LOG_COUNTERS,
	},
};
static const size_t PERF_LOG_NUM_COUNTERS = sizeof PERF_LOG_COUNTERS / sizeof *PERF_LOG_COUNTERS;
typedef struct {
	uint64_t nr;
	uint64_t instrs;
	uint64_t bandwidth;
} perf_read_format_t;

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

static inline clock_t realclock(void) {
	struct timespec ns;
	clock_gettime(CLOCK_REALTIME, &ns);
	return ns.tv_sec * 1000000 + ns.tv_nsec / 1000;
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
		int capacity_contracted, int capacity_expanded, bool huge, rng_t *randomize,
		rng_t *cont_rand, FILE *perflog) {
	int ret = 0;

	if(cont_rand)
		assert(randomize);

	uint8_t *large = alloc(capacity_expanded, huge);
	if(!large) { // "at large"
		perror("Allocating large array");
		ret = 1;
		goto cleanup;
	}

	int perfd = -1;
	clock_t startperf = 0;
	perf_read_format_t counters = {.nr = PERF_LOG_NUM_COUNTERS};
	if(perflog) {
		assert(CLOCKS_PER_SEC == 1000000);

		perfd = perf_poll_init(PERF_LOG_NUM_COUNTERS, PERF_LOG_COUNTERS);
		if(perfd == -1) {
			ret = 1;
			goto cleanup;
		}

		fputs(PERF_LOG_FILE_HEADER_LINE, perflog);

		if(!perf_poll_start(perfd)) {
			perror("Starting performance recording");
			ret = 1;
			goto cleanup;
		}
		startperf = clock();
	}
		
	for(int cycle = 0; cycle < 2 * num_periods; ++cycle) {
		const char *desc = "";
		rng_t *randomizer = randomize;
		int siz;
		uint8_t val = rand();

		if(cycle % 2) {
			siz = capacity_expanded;
			desc = "saturated";
		} else {
			siz = capacity_contracted;
			desc = "unsaturated";
			if(randomize && cont_rand)
				randomizer = cont_rand;
		}
		printf("Beginning %s passes\n", desc);

		clock_t startpass = clock();

		for(int pass = 0; pass < passes_per_cycle; ++pass) {
			bool seen_initial = false;
			for(int offset = 0; offset < siz; offset += cache_line_size) {
				unsigned ix;
				do {
					ix = randomize ? rng_lcfc(randomizer) * cache_line_size :
							(unsigned) offset;
				} while(ix >= (unsigned) siz);
				if(!ix)
					seen_initial = true;

				large[ix] ^= val;
				val ^= large[ix];
				large[ix] ^= val;
			}
			assert(!siz || seen_initial);

			if(perflog) {
				perf_poll_stop(perfd);
				ssize_t amt = read(perfd, &counters, sizeof counters);
				assert(amt == sizeof counters);
				assert(counters.nr == PERF_LOG_NUM_COUNTERS);
				fprintf(perflog, "%ld,%ld,%" PRIu64 ",%" PRIu64 "\n",
						realclock(), clock() - startperf,
						counters.instrs, counters.bandwidth);
				perf_poll_start(perfd);
			}
		}

		clock_t duration = clock() - startpass;
		printf("Completed iteration in %.6f seconds\n", ((double) duration) / CLOCKS_PER_SEC);
	}

	if(perflog && !perf_poll_stop(perfd))
		perror("Stopping performance recording");

cleanup:
	if(large)
		dealloc(capacity_expanded, large, huge);
	return ret;
}

int main(int argc, char *argv[]) {
	int ret = 0;

	rng_t *random = NULL;
	rng_t *randtv = NULL;
	FILE *perflog = NULL;
	int num_periods = DEFAULT_NUM_PERIODS;
	int passes_per_cycle = DEFAULT_PASSES_PER_CYCLE;
	int percent_contracted = DEFAULT_PERCENT_CONTRACTED;
	int percent_expanded = DEFAULT_PERCENT_EXPANDED;
	int custom_increment = 0;
	bool hugepages = false;
	bool randomize = false;
	bool secondrng = true;

	char *invoc = argv[0];
	int each_arg;
	opterr = 0;
	while((each_arg = getopt(argc, argv, "n:p:c:e:hr::sl:")) != -1) {
		switch(each_arg) {
		case 'n':
			if(!parse_arg_arg(each_arg, &num_periods)) {
				ret = 1;
				goto cleanup;
			}
			break;
		case 'p':
			if(!parse_arg_arg(each_arg, &passes_per_cycle)) {
				ret = 1;
				goto cleanup;
			}
			break;
		case 'c':
			if(!parse_arg_arg(each_arg, &percent_contracted)) {
				ret = 1;
				goto cleanup;
			}
			break;
		case 'e':
			if(!parse_arg_arg(each_arg, &percent_expanded)) {
				ret = 1;
				goto cleanup;
			}
			break;
		case 'h':
			hugepages = true;
			break;
		case 'r':
			randomize = true;
			if(optarg) {
				if(!parse_arg_arg(each_arg, &custom_increment)) {
					ret = 1;
					goto cleanup;
				}
			}
			break;
		case 's':
			secondrng = false;
			break;
		case 'l':
			perflog = fopen(optarg, "w");
			if(!perflog) {
				perror("Opening log file for writing");
				ret = 1;
				goto cleanup;
			}
			break;
		default:
			printf("USAGE: %s [-n #] [-p #] [-c %%] [-e %%] [-r [#]] [-s] [-l @]\n",
					invoc);
			printf(
					" -n #: Number of PERIODS (default %d)\n"
					" -p #: Number of PASSES per period (default %d)\n"
					" -c %%: Percent cache CONTRACTED 1/2-period (default %d)\n"
					" -e %%: Percent cache EXPANDED 1/2-period (default %d)\n"
					" -h: Allocate using huge pages for contiguous memory\n"
					" -r [#]: Randomize ordering to fool prefetcher\n"
					"         Optionally takes a custom rng increment\n"
					" -s: Use only one rng (default 2 w/ different periods)\n"
					" -l @: Log performance data to specified filename\n"
					"       Currently records: %s",
					DEFAULT_NUM_PERIODS, DEFAULT_PASSES_PER_CYCLE,
					DEFAULT_PERCENT_CONTRACTED, DEFAULT_PERCENT_EXPANDED,
					PERF_LOG_FILE_HEADER_LINE);
			ret = 1;
			goto cleanup;
		}
	}
	if(!secondrng && !randomize) {
		fputs("-s only makes sense in concert with -r\n", stderr);
		ret = 1;
		goto cleanup;
	}
	if(percent_contracted > percent_expanded) {
		fprintf(stderr, "Percent contracted (%d) is larger than percent expanded (%d)\n",
				percent_contracted, percent_expanded);
		ret = 1;
		goto cleanup;
	} else if(percent_contracted == percent_expanded)
		secondrng = false;

	llc_init();
	int cache_line_size = llc_line_size();
	int cache_num_sets = llc_num_sets();
	llc_cleanup();
	if(cache_line_size <= 0 || cache_num_sets <= 0) {
		ret = 1;
		goto cleanup;
	}

	int lines_contracted = cache_num_sets * percent_contracted / 100.0;
	int lines_expanded = cache_num_sets * percent_expanded / 100.0;

	const int TWO = 2;
	if(randomize) {
		if(!pow_of_two(cache_num_sets)) {
			fprintf(stderr, "Number of cache sets (%d) is not a power of two\n",
					cache_num_sets);
			ret = 1;
			goto cleanup;
		}

		unsigned siz = cache_num_sets;
		while(siz >> 1 >= (unsigned) lines_expanded && siz >> 1 >= MINIMUM_GENERATOR_PERIOD)
			siz >>= 1;
		while(siz << 1 <= (unsigned) lines_expanded)
			siz <<= 1;

		unsigned siz_reduced = siz;
		if(secondrng)
			while(siz_reduced >> 1 >= (unsigned) lines_contracted &&
					siz_reduced >> 1 >= MINIMUM_GENERATOR_PERIOD)
				siz_reduced >>= 1;

		if(custom_increment) {
			random = rng_lcfc_init_incr(siz, 1, &TWO, custom_increment);
			if(!random) {
				fprintf(stderr, "Provided rng increment (%d) not coprime with %d\n",
						custom_increment, cache_num_sets);
				ret = 1;
				goto cleanup;
			}
			if(secondrng) {
				randtv = rng_lcfc_init_incr(siz_reduced, 1, &TWO, custom_increment);
				assert(randtv);
			}
		} else {
			random = rng_lcfc_init(siz, 1, &TWO);
			if(!random) {
				perror("Allocating random number generator");
				ret = 1;
				goto cleanup;
			}
			if(secondrng) {
				randtv = rng_lcfc_init(siz_reduced, 1, &TWO);
				assert(randtv);
			}
		}
	}

	int cap_contracted = cache_line_size * lines_contracted;
	int cap_expanded = cache_line_size * lines_expanded;
	ret = square_evictions(cache_line_size, num_periods, passes_per_cycle,
			cap_contracted, cap_expanded, hugepages, random, randtv, perflog);

cleanup:
	if(random)
		rng_lcfc_clean(random);
	if(randtv)
		rng_lcfc_clean(randtv);
	if(perflog)
		fclose(perflog);
	return ret;
}

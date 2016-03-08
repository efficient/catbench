#include "llc.h"

#include <sys/mman.h>
#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "movnt.h"
#include "perf_poll.h"
#include "../rdtscp.h"
#include "rng.h"

#define DEFAULT_NUM_PERIODS        2
#define DEFAULT_PASSES_PER_CYCLE   100000
#define DEFAULT_PERCENT_CONTRACTED 80
#define DEFAULT_PERCENT_EXPANDED   220

#define MAX_TRUE_UNROLL            16

#define MAX_MEMREF_CYCLES_POW	17
#define LOWER(x) (1 << x)
#define UPPER(x) (1 << (x+1))
uint64_t histogram[MAX_MEMREF_CYCLES_POW] = {0};

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

#define PERF_LOG_BUFFER_INIT_SIZE (5 * 1024 * 1024)
#define PERF_LOG_BUFFER_GROW_FACT 2
typedef struct {
	uint64_t realtime;
	uint64_t cputime;
	uint64_t instrs;
	uint64_t bandwidth;
} perf_log_buffer_entry_t;
typedef struct {
	size_t siz;
	size_t occ;
	perf_log_buffer_entry_t log[];
} perf_log_buffer_t;

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

static inline bool bufappend(perf_log_buffer_t **buf, int perfd, clock_t startperf) {
	perf_read_format_t counters;
	perf_poll_stop(perfd);

	if((*buf)->occ + sizeof *(*buf)->log > (*buf)->siz) {
		puts("Outgrew performance logging buffer; expanding it...");
		perf_log_buffer_t *newloc = mremap(*buf, (*buf)->siz,
				PERF_LOG_BUFFER_GROW_FACT * (*buf)->siz, MREMAP_MAYMOVE);
		if(newloc == MAP_FAILED) {
			perror("Expanding in-memory log buffer");
			return false;
		}
		*buf = newloc;
		(*buf)->siz *= PERF_LOG_BUFFER_GROW_FACT;
	}

	ssize_t amt = read(perfd, &counters, sizeof counters);
	assert(amt == sizeof counters);
	assert(counters.nr == PERF_LOG_NUM_COUNTERS);

	movntq4((uint64_t *) ((uintptr_t) *buf + (*buf)->occ), realclock(), clock() - startperf,
			counters.instrs, counters.bandwidth);
	(*buf)->occ += sizeof *(*buf)->log;

	perf_poll_start(perfd);
	return true;
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

static bool uninterrupted = true;
static struct {
	uint8_t *arr;
	int cache_line_size;
	int num_periods;
	int passes_per_cycle;
	int capacity_contracted;
	int capacity_expanded;
	int unroll;
	rng_t *randomize;
	rng_t *cont_rand;
	bool no_randspin;
	bool check_memrate;
	perf_log_buffer_t **perflog;
	int perfstride;
	bool measure_all;
	int max_accesses;
} square_evictions_saved;

static void square_evictions_handler(int);
static void term_handler(int);

typedef int (*evictions_fun_t)(uint8_t *, int, int, int, int, int, int, rng_t *, rng_t *, bool,
		bool, perf_log_buffer_t **, int, bool, int);

static int square_evictions_async(uint8_t *arr, int cache_line_size, int num_periods,
		int passes_per_cycle, int capacity_contracted, int capacity_expanded, int unroll,
		rng_t *randomize, rng_t *cont_rand, bool no_randspin, bool check_memrate,
		perf_log_buffer_t **perflog, int perfstride, bool measure_all, int max_accesses) {
	square_evictions_saved.arr = arr;
	square_evictions_saved.cache_line_size = cache_line_size;
	square_evictions_saved.num_periods = num_periods;
	square_evictions_saved.passes_per_cycle = passes_per_cycle;
	square_evictions_saved.capacity_contracted = capacity_contracted;
	square_evictions_saved.capacity_expanded = capacity_expanded;
	square_evictions_saved.unroll = unroll;
	square_evictions_saved.randomize = randomize;
	square_evictions_saved.cont_rand = cont_rand;
	square_evictions_saved.no_randspin = no_randspin;
	square_evictions_saved.check_memrate = check_memrate;
	square_evictions_saved.perflog = perflog;
	square_evictions_saved.perfstride = perfstride;
	square_evictions_saved.measure_all = measure_all;
	square_evictions_saved.max_accesses = max_accesses;

	struct sigaction signal_setup = {.sa_handler = square_evictions_handler};
	struct sigaction term_setup = {.sa_handler = term_handler};
	if(sigaction(SIGUSR1, &signal_setup, NULL) != 0 ||
			sigaction(SIGTERM, &term_setup, NULL) != 0) {
		perror("Setting up signal handler");
		return 1;
	}

	while(uninterrupted);
	return 0;
}

static int square_evictions(uint8_t *arr, int cache_line_size, int num_periods,
		int passes_per_cycle, int capacity_contracted, int capacity_expanded, int unroll,
		rng_t *randomize, rng_t *cont_rand, bool no_randspin, bool check_memrate,
		perf_log_buffer_t **perflog, int perfstride, bool measure_all, int max_accesses) {
	assert(perflog);

	if(cont_rand)
		assert(randomize);

	int perfd = -1;
	clock_t startperf = 0;
	if(*perflog) {
		assert(CLOCKS_PER_SEC == 1000000);

		if(perfstride == -1)
			perfstride = capacity_expanded / cache_line_size;

		perfd = perf_poll_init(PERF_LOG_NUM_COUNTERS, PERF_LOG_COUNTERS);
		if(perfd == -1)
			return 1;

		if(!perf_poll_start(perfd)) {
			perror("Starting performance recording");
			return 1;
		}
		startperf = clock();
	}
		
	clock_t duration = 0;
	for(int cycle = 0; cycle < 2 * num_periods; ++cycle) {
		const char *desc = "";
		rng_t *randomizer = randomize;
		int siz;
		int accesses = 0;
		uint64_t time = 0;
		uint64_t max_time = 0;
		uint8_t val[unroll];

		for(int idx = 0; idx < unroll; ++idx)
			val[idx] = rand();

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
				int subaccesses = (siz - offset) / cache_line_size;
				if(subaccesses > unroll)
					subaccesses = unroll;
				if(max_accesses && accesses + subaccesses > max_accesses)
					subaccesses = max_accesses - accesses;

				unsigned ix[subaccesses];
				for(int idx = 0; idx < subaccesses; ++idx) {
					int tries = 0;
					unsigned thisix;
					do {
						thisix = randomize ? rng_lcfc(randomizer) * cache_line_size :
								(unsigned) offset + idx * cache_line_size;
						++tries;
					} while(thisix >= (unsigned) siz);
					if(!thisix)
						seen_initial = true;
					if(no_randspin && tries > 1) {
						fprintf(stderr, "ERROR: Took %d tries before we got an in-range random!\n", tries);
						return 1;
					}

					ix[idx] = thisix;
				}

				if(measure_all)
					time = rdtscp();

				int subaccess = 0;
				switch(subaccesses) {
				default:
					while(subaccesses - subaccess > MAX_TRUE_UNROLL)
						arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 16:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 15:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 14:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 13:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 12:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 11:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 10:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 9:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 8:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 7:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 6:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 5:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 4:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 3:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 2:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 1:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				};

				subaccess = 0;
				switch(subaccesses) {
				default:
					while(subaccesses - subaccess > MAX_TRUE_UNROLL)
						val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 16:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 15:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 14:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 13:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 12:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 11:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 10:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 9:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 8:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 7:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 6:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 5:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 4:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 3:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 2:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				case 1:
					val[subaccess] ^= arr[ix[subaccess]], ++subaccess;
				};

				subaccess = 0;
				switch(subaccesses) {
				default:
					while(subaccesses - subaccess > MAX_TRUE_UNROLL)
						arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 16:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 15:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 14:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 13:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 12:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 11:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 10:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 9:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 8:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 7:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 6:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 5:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 4:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 3:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 2:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				case 1:
					arr[ix[subaccess]] ^= val[subaccess], ++subaccess;
				};

				if(measure_all) {
					time = (rdtscp() - time) / subaccesses;
					if(time > max_time) {
						max_time = time;
						printf("Current max cycles for a memory access %lu\n", max_time);
					}
					// Ignore the first 2 cycles due to memory fetch behavior
					if(cycle < 2) {
						++histogram[logtwo(time)];
					}
				}

				if(*perflog && offset / cache_line_size % perfstride == 0)
					if(!bufappend(perflog, perfd, startperf))
						return 1;

				accesses += subaccesses;
				if(accesses == max_accesses) {
					duration += clock() - startpass;
					printf("Completed traversal in %.6f seconds\n", ((double) duration) / CLOCKS_PER_SEC);
					fflush(stdout);
					if(check_memrate)
						fprintf(stderr, "Sanity check: %.6f accesses/s\n", (double) accesses / duration * CLOCKS_PER_SEC);
					goto breakoutermost;
				}

				offset += (subaccesses - 1) * cache_line_size;
			}
			assert(!siz || seen_initial);
		}

		if(!max_accesses) {
			clock_t duration = clock() - startpass;
			printf("Completed iteration in %.6f seconds\n", ((double) duration) / CLOCKS_PER_SEC);
			fflush(stdout);
			if(check_memrate) {
				fprintf(stderr, "Accesses performed: %d\n", accesses);
				fprintf(stderr, "Sanity check: %.6f accesses/s\n", (double) accesses / duration * CLOCKS_PER_SEC);
			}
		} else
			duration += clock() - startpass;
	}
breakoutermost:

	if(*perflog && !perf_poll_stop(perfd))
		perror("Stopping performance recording");

	return 0;
}

static void square_evictions_handler(int sig) {
	(void) sig;
	square_evictions(square_evictions_saved.arr,
			square_evictions_saved.cache_line_size,
			square_evictions_saved.num_periods,
			square_evictions_saved.passes_per_cycle,
			square_evictions_saved.capacity_contracted,
			square_evictions_saved.capacity_expanded,
			square_evictions_saved.unroll,
			square_evictions_saved.randomize,
			square_evictions_saved.cont_rand,
			square_evictions_saved.no_randspin,
			square_evictions_saved.check_memrate,
			square_evictions_saved.perflog,
			square_evictions_saved.perfstride,
			square_evictions_saved.measure_all,
			square_evictions_saved.max_accesses);
}

static void term_handler(int sig) {
	(void) sig;
	uninterrupted = false;
}

int main(int argc, char *argv[]) {
	int ret = 0;

	uint8_t *large = NULL;
	int *rng_factors = NULL;
	rng_t *random = NULL;
	rng_t *randtv = NULL;
	FILE *perflog = NULL;
	perf_log_buffer_t *perfbuf = NULL;
	size_t nrng_factors = 0;
	int num_periods = DEFAULT_NUM_PERIODS;
	int passes_per_cycle = DEFAULT_PASSES_PER_CYCLE;
	int percent_contracted = DEFAULT_PERCENT_CONTRACTED;
	int percent_expanded = DEFAULT_PERCENT_EXPANDED;
	int unroll = 1;
	int custom_increment = 0;
	int perfstride = -1;
	int accesses = 0;
	int baserandom = 0;
	bool hugepages = false;
	bool randomize = false;
	bool secondrng = true;
	bool norngspin = false;
	bool sanitycheck_memory = false;
	bool sanitycheck_clockspeed = false;
	bool measure_all = false;
	bool await_signal = false;

	char *invoc = argv[0];
	int each_arg;
	opterr = 0;
	while((each_arg = getopt(argc, argv, "n:p:c:e:hu:r::qf:swvl:j:ma:b:i")) != -1) {
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
		case 'u':
			if(!parse_arg_arg(each_arg, &unroll)) {
				ret = 1;
				goto cleanup;
			}
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
		case 'q':
			norngspin = true;
			break;
		case 'f': {
			char *factors = optarg;
			nrng_factors = 1;
			for(char *chr = factors; *chr; ++chr)
				if(*chr == ',')
					++nrng_factors;

			rng_factors = malloc(nrng_factors * sizeof *rng_factors);
			int *dest = rng_factors;
			while(sscanf(factors, "%d,", dest++)) {
				char *repl = strchr(factors, ',');
				if(repl)
					factors = repl + 1;
				else {
					sscanf(factors, "%d", dest - 1);
					break;
				}
			}
			break;
		}
		case 's':
			secondrng = false;
			break;
		case 'w':
			sanitycheck_memory = true;
			break;
		case 'v':
			sanitycheck_clockspeed = true;
			break;
		case 'l':
			perflog = fopen(optarg, "w");
			if(!perflog) {
				perror("Opening log file for writing");
				ret = 1;
				goto cleanup;
			}
			break;
		case 'j':
			if(!parse_arg_arg(each_arg, &perfstride)) {
				ret = 1;
				goto cleanup;
			}
			if(perfstride <= 0) {
				fputs("-j expects a positive integer\n", stderr);
				ret = 1;
				goto cleanup;
			}
			break;
		case 'm':
			measure_all = true;
			break;
		case 'a':
			if(!parse_arg_arg(each_arg, &accesses)) {
				ret = 1;
				goto cleanup;
			}
			break;
		case 'b':
			if(!parse_arg_arg(each_arg, &baserandom)) {
				ret = 1;
				goto cleanup;
			}
			break;
		case 'i':
			await_signal = true;
			break;
		default:
			printf("USAGE: %s [-n #] [-p #] [-c %%] [-e %%] [-h] [-u] [-r [#]] [-q] [-f ,,] [-s] [-w] [-v] [-l @] [-j #] [-m] [-a #] [-b #] [-i]\n",
					invoc);
			printf(
					" -n #: Number of PERIODS (default %d)\n"
					" -p #: Number of PASSES per period (default %d)\n"
					" -c %%: Percent cache CONTRACTED 1/2-period (default %d)\n"
					" -e %%: Percent cache EXPANDED 1/2-period (default %d)\n"
					" -h: Allocate using huge pages for contiguous memory\n"
					" -u #: Unroll loop given number of times\n"
					" -r [#]: Randomize ordering to fool prefetcher\n"
					"         Optionally takes a custom rng increment\n"
					" -q: Do not spin to handle out-of-bounds rng results\n"
					" -f ,,: List prime factors of expanded cache line count\n"
					" -s: Use only one rng (default 2 w/ different periods)\n"
					" -w: Output sanity-check memory throughput on stderr\n"
					" -v: Output sanity-check clock speed guess on stderr\n"
					" -l @: Log performance data to specified filename\n"
					"       Currently records: %s"
					" -j #: Logging recording stride (default once per pass)\n"
					" -m: MEAsure all memory accesses using rdtscp\n"
					" -a #: Limit accesses to a subset of this cardinality\n"
					" -b #: Perform first access this far into the rng period\n"
					" -i: Wait and perform the specified traversal on USR1\n",
					DEFAULT_NUM_PERIODS, DEFAULT_PASSES_PER_CYCLE,
					DEFAULT_PERCENT_CONTRACTED, DEFAULT_PERCENT_EXPANDED,
					PERF_LOG_FILE_HEADER_LINE);
			ret = 1;
			goto cleanup;
		}
	}
	if((!secondrng || rng_factors) && !randomize) {
		fputs("-f and -s only makes sense in concert with -r\n", stderr);
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
	if(perflog || sanitycheck_clockspeed) {
		perfbuf = mmap(0, PERF_LOG_BUFFER_INIT_SIZE, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
		if(perfbuf == MAP_FAILED) {
			puts("Allocating in-memory log buffer");
			perfbuf = NULL;
			ret = 1;
			goto cleanup;
		}
		perfbuf->siz = PERF_LOG_BUFFER_INIT_SIZE;
		perfbuf->occ = (uintptr_t) perfbuf->log - (uintptr_t) perfbuf;
	} else if(perfstride != -1) {
		fputs("-j only makes sense in concert with -l or -v\n", stderr);
		ret = 1;
		goto cleanup;
	}
	if((accesses || baserandom) && !randomize) {
		fputs("-a and -b only make sense in concert with -r\n", stderr);
		ret = 1;
		goto cleanup;
	}
	if(unroll > MAX_TRUE_UNROLL)
		fprintf(stderr, "WARN: You requested unrolling groups of %d accesses, but I can "
				"only *really* do this up to %d at a time. I'll still group your "
				"accesses for you, but the excess ones might be a bit slower.\n",
				unroll, MAX_TRUE_UNROLL);

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

	if(randomize) {
		if(!pow_of_two(cache_num_sets)) {
			fprintf(stderr, "Number of cache sets (%d) is not a power of two\n",
					cache_num_sets);
			ret = 1;
			goto cleanup;
		}

		unsigned siz_reduced = cache_num_sets;
		while(siz_reduced >> 1 >= (unsigned) lines_contracted &&
				siz_reduced >> 1 >= MINIMUM_GENERATOR_PERIOD)
			siz_reduced >>= 1;

		const int TWO = 2;
		size_t nfactors;
		const int *factors = NULL;
		unsigned siz;
		if(rng_factors) {
			nfactors = nrng_factors;
			factors = rng_factors;
			siz = lines_expanded;

			int check = 1;
			for(const int *factor = factors; factor < factors + nfactors; ++factor)
				check *= *factor;
			if(lines_expanded % check) {
				fprintf(stderr, "Provided factorization is not correct for %d\n", lines_expanded);
				ret = 1;
				goto cleanup;
			}
		} else {
			nfactors = 1;
			factors = &TWO;
			siz = siz_reduced;
			while(siz < (unsigned) lines_expanded)
				siz <<= 1;
		}

		if(custom_increment) {
			random = rng_lcfc_init_incr(siz, nfactors, factors, custom_increment);
			if(!random) {
				fprintf(stderr, "Provided rng increment (%d) not coprime with %d\n",
						custom_increment, cache_num_sets);
				ret = 1;
				goto cleanup;
			}
		} else {
			random = rng_lcfc_init(siz, nfactors, factors);
			if(!random) {
				perror("Allocating random number generator");
				ret = 1;
				goto cleanup;
			}
		}
		if(secondrng) {
			if(custom_increment)
				randtv = rng_lcfc_init_incr(siz_reduced, 1, &TWO, custom_increment);
			else
				randtv = rng_lcfc_init(siz_reduced, 1, &TWO);
			assert(randtv);
		}

		for(int wasted = 0; wasted < baserandom; ++wasted) {
			rng_lcfc(random);
			if(secondrng)
				rng_lcfc(randtv);
		}
	}

	int cap_contracted = cache_line_size * lines_contracted;
	int cap_expanded = cache_line_size * lines_expanded;
	evictions_fun_t call = square_evictions;
	if(await_signal)
		call = square_evictions_async;

	large = alloc(cap_expanded, hugepages);
	if(!large) { // "at large"
		perror("Allocating large array");
		ret = 1;
		goto cleanup;
	}
	ret = call(large, cache_line_size, num_periods, passes_per_cycle, cap_contracted,
			cap_expanded, unroll, random, randtv, norngspin, sanitycheck_memory,
			&perfbuf, perfstride, measure_all, accesses);

	if(perflog) {
		assert(perfbuf);
		fputs(PERF_LOG_FILE_HEADER_LINE, perflog);
		for(perf_log_buffer_entry_t *entry = perfbuf->log;
				(uintptr_t) entry < (uintptr_t) perfbuf + perfbuf->occ; ++entry)
			fprintf(perflog, "%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
					entry->realtime, entry->cputime,
					entry->instrs, entry->bandwidth);
	}
	if(sanitycheck_clockspeed) {
		assert(perfbuf);
		uint64_t instrs = 0;
		uint64_t cputime = 0;
		for(perf_log_buffer_entry_t *entry = perfbuf->log;
				(uintptr_t) entry < (uintptr_t) perfbuf + perfbuf->occ; ++entry) {
			instrs += entry->instrs;
			cputime += entry->cputime;
		}
		fprintf(stderr, "Instructions executed: %" PRIu64 "\n", instrs);
		fprintf(stderr, "Sanity check: %.6f Hz\n", (double) instrs / cputime * 1000000);
	}

cleanup:
	if(large)
		dealloc(cap_expanded, large, hugepages);
	if(rng_factors)
		free(rng_factors);
	if(random)
		rng_lcfc_clean(random);
	if(randtv)
		rng_lcfc_clean(randtv);
	if(perfbuf)
		munmap(perfbuf, perfbuf->siz);
	if(perflog)
		fclose(perflog);
	if(measure_all) {
		for(int i = 0; i < MAX_MEMREF_CYCLES_POW; ++i) {
			printf("Mem access cycles >= 2^%d: %lu\n", i, histogram[i]);
		}
	}
	return ret;
}

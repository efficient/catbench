#include "llc.h"

#include <sys/mman.h>
#include <assert.h>
#include <pqos.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_NUM_PASSES      100000
#define DEFAULT_PERCENT         80
#define DEFAULT_START_CYCLE     0
/* TODO: These are probably not super right. Do the right
   thing once everything kinda-works */
#define NUM_WAYS 12
#define NUM_CORES 8

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

static void rotate_cores(int loc) {
  for(int i = 0; i < NUM_CORES; i++) {
    int ret = pqos_l3ca_assoc_set(i, (i+loc)%NUM_WAYS);
    assert(ret == PQOS_RETVAL_OK);
  }
  sleep(2);
}

// Store the precise number of cycles different between each offset
static uint64_t config_offset_clocks[NUM_WAYS];

static int dist_a(int cache_line_size, int num_passes, int capacity, int start_cycle) {
  uint8_t *large = alloc(capacity, 1);
  assert(large != NULL);
  struct pqos_l3ca cos[NUM_WAYS];
  for (int i=0; i < NUM_WAYS; i++) {
    cos[i].class_id = i;
    cos[i].cdp = false;
    cos[i].ways_mask = 1 << i;
    }
  pqos_l3ca_set(0, NUM_WAYS, cos);

  if(!large) { // "at large"
    perror("Allocating large array");
    return 1;
  }
		
  for(int offset = 0; offset < NUM_WAYS; ++offset) {
    int cycle = (start_cycle + offset) % NUM_WAYS;
    int size = capacity;
    uint8_t val = rand();

    rotate_cores(cycle);
    printf("Beginning passes: Offset %d: \n", cycle);

    uint64_t begin = rdtscp();
    for(int pass = 0; pass < num_passes; ++pass)
      for(int offset = 0; offset < size; offset += cache_line_size) {
        large[offset] ^= val;
        val ^= large[offset];
        large[offset] ^= val;
      }
    uint64_t end = rdtscp();

    config_offset_clocks[cycle] = end - begin;
  }

  #define BASE_CYCLES config_offset_clocks[0]
  uint64_t config_offset_diffs[NUM_WAYS];
  printf("Number of passes: %d\n", num_passes);
  for(int cycle = 0; cycle < NUM_WAYS; ++cycle) {
        config_offset_diffs[cycle] = config_offset_clocks[cycle] - BASE_CYCLES;
        //printf("Pass %d normalized cycles: %lu", cycle, config_offset_diffs[cycle]);
	printf("%lu\n", config_offset_diffs[cycle]);
        //printf(" %s\n", cycle == start_cycle ? "(initial cycle)" : "");
  }
  #undef BASE_CYCLES

  dealloc(capacity, large, 1);
  return 0;
}

int main(int argc, char *argv[]) {
  int num_passes = DEFAULT_NUM_PASSES;
  int percent = DEFAULT_PERCENT;
  int start_cycle = DEFAULT_START_CYCLE;
  char *invoc = argv[0];
  int each_arg;

  opterr = 0;
  while((each_arg = getopt(argc, argv, "n:p:s:")) != -1) {
    switch(each_arg) {
    case 'n':
      if(!parse_arg_arg(each_arg, &num_passes))
        return 1;
      break;
    case 'p':
      if(!parse_arg_arg(each_arg, &percent))
        return 1;
      break;
    case 's':
      if(!parse_arg_arg(each_arg, &start_cycle))
        return 1;
      break;
    default:
      printf("USAGE: %s [-n #] [-p #]\n", invoc);
      printf(
             " -n #: Number of passes (default %d)\n"
             " -p #: Percentage of cache in working set (default %d)\n"
             " -s #: Which way to start with\n",
             DEFAULT_NUM_PASSES, DEFAULT_PERCENT);
      return 1;
    }
  }

  llc_init();
  int cache_line_size = llc_line_size();
  int cache_num_sets = llc_num_sets();
  llc_cleanup();
  if(cache_line_size <= 0 || cache_num_sets <= 0)
    return 1;

  int cap = cache_line_size * cache_num_sets * percent / 100.0;

  struct pqos_config cfg = {
    .fd_log = stderr,
    .verbose = 0,
    .topology = NULL,
    .free_in_use_rmid = 0,
    .cdp_cfg = PQOS_REQUIRE_CDP_OFF,
  };
  int pqos_res = pqos_init(&cfg);
  if(pqos_res != PQOS_RETVAL_OK) {
    return pqos_res;
  }

  /* TODO: return error condition instead of assertion */
  rotate_cores(0);
  int ret = dist_a(cache_line_size, num_passes, cap, start_cycle);
  pqos_fini();
  return ret;
}

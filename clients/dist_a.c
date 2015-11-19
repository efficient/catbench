#include "llc.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define DEFAULT_NUM_PASSES   100000
#define DEFAULT_PERCENT 80
#define NUM_WAYS 12

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
}

static int square_evictions(int cache_line_size, int num_passes, int capacity) {
  uint8_t *large = malloc(capacity_expanded);
  if(!large) { // "at large"
    perror("Allocating large array");
    return 1;
  }
		
  for(int cycle = 0; cycle < NUM_WAYS; ++cycle) {
    int siz;
    uint8_t val = rand();

    rotate_cores(cycle);
    printf("Beginning passes: Offset %d: \n", cycle);
    clock_t now = clock();
    for(int pass = 0; pass < passes_per_cycle; ++pass)
      for(int offset = 0; offset < siz; offset += cache_line_size) {
        large[offset] ^= val;
        val ^= large[offset];
        large[offset] ^= val;
      }
    clock_t then = clock();
    printf("Pass %d took %d seconds:\n", cycle, (then - now)/CLOCKS_PER_SEC);
  }

  free(large);
  return 0;
}

int main(int argc, char *argv[]) {
  int num_passes = DEFAULT_NUM_PASSES;
  int percent = DEFAULT_PERCENT;
  char *invoc = argv[0];
  int each_arg;

  opterr = 0;
  while((each_arg = getopt(argc, argv, "n:p:")) != -1) {
    switch(each_arg) {
    case 'n':
      if(!parse_arg_arg(each_arg, &num_passes))
        return 1;
      break;
    case 'p':
      if(!parse_arg_arg(each_arg, &percent))
        return 1;
      break;
    default:
      printf("USAGE: %s [-n #] [-p #]\n", invoc);
      printf(
             " -n #: Number of passes (default %d)\n"
             " -p #: Percentage of cache in working set (default %d)\n",
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
  return square_evictions(cache_line_size, passes, cap);
}

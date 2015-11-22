#include "llc.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "../external/pqos/lib/pqos.h"

#define DEFAULT_NUM_PASSES   100000
#define DEFAULT_PERCENT 80
/* TODO: These are probably not super right. Do the right
   thing once everything kinda-works */
#define NUM_WAYS 12
#define NUM_CORES 8

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
    pqos_l3ca_assoc_set(i, (i+loc)%NUM_WAYS);
  }
}

static int square_evictions(int cache_line_size, int num_passes, int capacity) {
  uint8_t *large = malloc(capacity);
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
		
  for(int cycle = 0; cycle < NUM_WAYS; ++cycle) {
    int size = capacity;
    uint8_t val = rand();

    rotate_cores(cycle);
    printf("Beginning passes: Offset %d: \n", cycle);
    clock_t now = clock();
    for(int pass = 0; pass < num_passes; ++pass)
      for(int offset = 0; offset < size; offset += cache_line_size) {
        large[offset] ^= val;
        val ^= large[offset];
        large[offset] ^= val;
      }
    clock_t then = clock();
    printf("Pass %d took %f seconds:\n", cycle, ((float)(then - now)/(float)CLOCKS_PER_SEC));
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
  return square_evictions(cache_line_size, num_passes, cap);
}
 #include "llc.h"

#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include <pqos.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../syscallers.h"

#define DEFAULT_NUM_PASSES      100000
#define DEFAULT_PERCENT         80
#define DEFAULT_START_CYCLE     0
/* TODO: These are probably not super right. Do the right
   thing once everything kinda-works */
#define NUM_WAYS 12
#define NUM_CORES 8


static inline uint64_t rdtscp(void) {
	uint32_t cycles_low, cycles_high;
	__asm volatile(
		"RDTSCP\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t"
		: "=r" (cycles_high), "=r" (cycles_low)
		:
		: "eax", "edx", "ecx"
		);
	return ((uint64_t)cycles_high << 32 | cycles_low);
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
}

// Store the precise number of cycles different between each offset
static uint64_t config_offset_clocks[NUM_WAYS];
static uint64_t config_offset_begin[NUM_WAYS];
static uint64_t config_offset_end[NUM_WAYS];

static int dist_a(int num_passes, int start_cycle) {
  struct pqos_l3ca cos[NUM_WAYS];
  for (int i=0; i < NUM_WAYS; i++) {
    cos[i].class_id = i;
    cos[i].cdp = false;
    cos[i].ways_mask = 1 << i;
    }
  pqos_l3ca_set(0, NUM_WAYS, cos);
		
  for(int offset = 0; offset < NUM_WAYS; ++offset) {
    int cycle = (start_cycle + offset) % NUM_WAYS;
    rotate_cores(cycle);
    const char *cmdline[] = {"clients/square_evictions", "-n1", "-c5", "-e5", "-p100000", "-r", NULL};
    /* Begin timed section */
    uint64_t begin = rdtscp();
    pid_t pid = fork();
    switch(pid) {
    case -1:
      perror("Forking child");
      break;
    case 0:
      exec_v(cmdline[0], cmdline);
      perror("execing square_evictions\n");
      return -1;
    default:
      waitpid(pid, NULL, 0);
    }
    uint64_t end = rdtscp();
    /* End timed section */
    assert(end > begin);
    config_offset_clocks[cycle] = end - begin;
    config_offset_begin[cycle] = begin;
    config_offset_end[cycle] = end;
  }

  printf("Number of passes: %d\n", num_passes);
  for(int cycle = 0; cycle < NUM_WAYS; ++cycle) {
    printf("DISTA REPORTING CYCLE %d: %lu\n", cycle, config_offset_clocks[cycle]);
  }

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

  llc_init();
  int cache_num_sets = llc_num_sets();
  llc_cleanup();
  if(cache_num_sets <= 0)
    return 1;
  printf("Num sets: %d\n", cache_num_sets);

  /* TODO: return error condition instead of assertion */
  rotate_cores(0);
  int ret = dist_a(num_passes, start_cycle);
  pqos_fini();
  return ret;
}

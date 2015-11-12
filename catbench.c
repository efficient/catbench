#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "cpu_support.h"
#include "log.h"
#include "proc_manip.h"

// Number of times to iterate over all processes when attempting to move them between CPUs
#define REARRANGE_RETRIES 3

static int pick_core (const cpu_support_t *feats, int procs_go_where) {
  int cpu = procs_go_where % feats->num_cores;
  while (cpu < 0) cpu += feats-> num_cores;
  return cpu;
}

static bool rearrange_processes(bool multicore, int procs_go_where, const cpu_support_t *feats) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	if(multicore) {
		for(int cpu = 0; cpu < feats->num_cores; ++cpu)
			CPU_SET(cpu, &mask);
	} else {
          CPU_SET(pick_core(feats, procs_go_where), &mask);
        }

	int attempts;
	uint8_t failed;
	// TODO: Only hold cap_sys_nice here
	for(attempts = 1; attempts <= REARRANGE_RETRIES &&
			proc_manip_rebalance_system(&failed, &mask); ++attempts) {
		log_msg(LOG_VERBOSE, "Relocation attempt #%d completed with code %#x\n",
				attempts, failed);
	}

	if(attempts > REARRANGE_RETRIES) {
		log_msg(LOG_ERROR, "Gave up before relocating all processes\n");
		return false;
	}
	if(failed) {
		log_msg(LOG_ERROR, "Final relocation attempt failed with code %#x\n", failed);
		return false;
	}
	log_msg(LOG_INFO, "Successfully relocated all system processes in %d attempts\n", attempts);
	return true;
}

static void* naptime(void* pointerToFalse) {
  log_msg(LOG_INFO, "want to sleep");
  sleep(3);
  log_msg(LOG_INFO, "sleppt");
  return pointerToFalse;
}

static void run_benchmarks(const cpu_support_t *feats) {
  log_msg(LOG_INFO, "On your mark");
  int nthreads = feats->num_cores;
  pthread_t threads[nthreads];
  void* (*tests[]) (void*) = {
    &naptime,
    NULL
};

  for (int i=0; tests[i]; i++) {
    log_msg(LOG_INFO, "Get benched");
    for (int j=0; j<nthreads; j++) {
      log_msg(LOG_INFO, "GOO");
      pthread_create(threads+j, NULL, tests[i], NULL);
    } 
    for (int j=0; j<nthreads; j++) {
      pthread_join(threads[j], NULL);
    } 
  }
  //TODO: Insert printification of results here.
}

int main(int argc, char** argv) {
        int ret = 0;
        int unpin_procs = 0;
        int procs_go_where = 0;
        int quit = 0;
	cpu_support_t feats;

        // Get dem opts
        int c, has_p = 0;
        while ((c = getopt(argc, argv, "hqup:")) != -1) {
          switch (c) {
          case 'u':
            unpin_procs = 1;
            break;
          case 'p':
            procs_go_where = atoi(optarg);
            has_p = 1;
            break;
          case 'q':
            quit = 1;
            break;
          case 'h':
            log_msg(LOG_INFO, 
"Usage: \n"
"  -p CORE: Pin system processes to CORE\n"
"  -u: Unpin all system processes\n"
"  -q: Quit after migrating, before benchmarks\n"
"  -h: Print this message and exit\n");
            return 0;
          }
        }

        if (has_p && unpin_procs) {
          log_msg(LOG_FATAL, "Cannot specify both -p and -u");
        }

	cpu_support(&feats);
	cpu_support_log(LOG_INFO, &feats);

	if(feats.num_cores == 1) {
		log_msg(LOG_FATAL, "Benchmarks must be run on a multiprocessor\n");
		ret = 1;
		goto cleanup;
	}
	if(!feats.num_cat_levels) {
		log_msg(LOG_FATAL, "Benchmarks require altogether missing CAT support\n");
		ret = 1;
		goto cleanup;
	}
	cpu_support_foreach_cat_level(cache_level, feats) {
		if (cache_level->cache_level != 3)
			continue;

		if (!cache_level->supported) {
			log_msg(LOG_FATAL, "Benchmarks require missing L3 CAT support\n");
			ret = 1;
			goto cleanup;
		}
		break;
	}

	if(!rearrange_processes(unpin_procs, procs_go_where, &feats)) {
		log_msg(LOG_FATAL, "Unable to relocate all system processes (permissions?)\n");
                // Probably don't want to quit early here, because failing to migrate is common
		// ret = 1;
		// goto cleanup;
	}
        if (quit) goto cleanup;
        run_benchmarks(&feats);

cleanup:
        rearrange_processes(true, 0, &feats);
	cpu_support_cleanup(&feats);
	return ret;
}

#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pqos.h>
#include <pthread.h>

#include "log.h"
#include "proc_manip.h"

// Number of times to iterate over all processes when attempting to move them between CPUs
#define REARRANGE_RETRIES 3

static int pick_core (const struct pqos_cpuinfo *feats, int procs_go_where) {
  int cpu = procs_go_where % feats->num_cores;
  while (cpu < 0) cpu += feats-> num_cores;
  return cpu;
}

static bool rearrange_processes(bool multicore, int procs_go_where,
		const struct pqos_cpuinfo *feats) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	if(multicore) {
		for(unsigned cpu = 0; cpu < feats->num_cores; ++cpu)
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

static void run_benchmarks(const struct pqos_cpuinfo *feats) {
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

	struct pqos_config cfg = {
		.fd_log = log_get_dest(),
		.verbose = log_get_verbosity(),
		.topology = NULL,
		.free_in_use_rmid = 0,
		.cdp_cfg = PQOS_REQUIRE_CDP_OFF,
	};
	int pqos_res = pqos_init(&cfg);
	if(pqos_res != PQOS_RETVAL_OK) {
		log_msg(LOG_FATAL, "Library init failed with code %d (HW support?)\n", pqos_res);
		ret = 1;
		goto cleanup;
	}

	const struct pqos_cpuinfo *traits = NULL;
	const struct pqos_cap *feats = NULL;
	pqos_cap_get(&feats, &traits);

	if(!rearrange_processes(unpin_procs, procs_go_where, traits)) {
		log_msg(LOG_FATAL, "Unable to relocate all system processes (permissions?)\n");
                // Probably don't want to quit early here, because failing to migrate is common
		// ret = 1;
		// goto cleanup;
	}
        if (quit) goto cleanup;
        run_benchmarks(traits);
	if(!unpin_procs)
		rearrange_processes(true, 0, traits);

cleanup:
	pqos_fini();
	return ret;
}

#include "prep_system.h"

#include <pqos.h>
#include <stdlib.h>

#include "log.h"
#include "proc_manip.h"

/* TODO: Fix prep_system first argument, clean_system argument */
static int pick_core (const struct pqos_cpuinfo *traits, int procs_go_where) {
	int cpu = procs_go_where % traits->num_cores;
	while (cpu < 0) cpu += traits-> num_cores;
	return cpu;
}

static bool rearrange_processes(bool multicore, int procs_go_where,
		const struct pqos_cpuinfo *traits) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	if(multicore) {
		for(unsigned cpu = 0; cpu < traits->num_cores; ++cpu)
			CPU_SET(cpu, &mask);
	} else {
		CPU_SET(pick_core(traits, procs_go_where), &mask);
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

#define GIT_LOG_CMD 	"git log --oneline -1"
#define GIT_STATUS_CMD  "git status -uno"
#define GIT_DIFF_CMD	"git --no-pager diff HEAD"
#define DELIM 		"==="
// Purposely ignoring the usual style for pointers!
// I don't know enough about PL to argue about it though!
int prep_system(bool multicore, int procs_go_where) {
	printf("%s\n", GIT_LOG_CMD);
	system(GIT_LOG_CMD);
	printf("%s\n", GIT_STATUS_CMD);
	system(GIT_STATUS_CMD);
	printf("%s\n", GIT_DIFF_CMD);
	system(GIT_DIFF_CMD);
	printf("%s\n", DELIM);
	int ret = 0;
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
		ret = -1;
	}
	const struct pqos_cpuinfo *traits = NULL;
	const struct pqos_cap *feats = NULL;
	pqos_cap_get(&feats, &traits);
	if(!rearrange_processes(multicore, procs_go_where, traits)) {
		log_msg(LOG_FATAL, "Unable to relocate all system processes (permissions?)\n");
		ret = -1;
	}
	return ret;
}

#undef GIT_LOG_CMD
#undef GIT_STATUS_CMD
#undef GIT_DIFF_CMD
#undef DELIM

// TODO add pqos_fini errors to return value
int cleanup_system(bool unpin_procs) {
	int ret = 0;
	const struct pqos_cpuinfo *traits = NULL;
	const struct pqos_cap *feats = NULL;
	pqos_cap_get(&feats, &traits);
	if(!unpin_procs) {
		bool err = rearrange_processes(true, 0, traits);
		if(err == false) {
			log_msg(LOG_FATAL, "Cleanup failed\n");
			ret = -1;
		}
	}
	pqos_fini();
	return ret;
}

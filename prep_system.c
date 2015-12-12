#include "prep_system.h"

#include <pqos.h>
#include <stdbool.h>

#include "log.h"
#include "proc_manip.h"

static int pick_core (const struct pqos_cpuinfo *traits, int procs_go_where) {
	int cpu = procs_go_where % traits->num_cores;
	while (cpu < 0) cpu += traits-> num_cores;
	return cpu;
}

bool rearrange_processes(bool multicore, int procs_go_where,
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

// Purposely ignoring the usual style for pointers!
// I don't know enough about PL to argue about it though!
int prep_system(bool multicore, int procs_go_where, const struct pqos_cpuinfo* traits) {
	int ret = 0;
	if(!rearrange_processes(multicore, procs_go_where, traits)) {
		log_msg(LOG_FATAL, "Unable to relocate all system processes (permissions?)\n");
		ret = -1;
	}
	return ret;
}

// TODO add pqos_fini errors to return value
int cleanup_system(bool unpin_procs, const struct pqos_cpuinfo* traits) {
	int ret = 0;
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

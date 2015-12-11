#include "prep_system.h"

#include <pqos.h>
#include <stdbool.h>

#include "log.h"
#include "proc_manip.h"

static int pick_core (const struct pqos_cpuinfo *feats, int procs_go_where) {
	int cpu = procs_go_where % feats->num_cores;
	while (cpu < 0) cpu += feats-> num_cores;
	return cpu;
}

bool rearrange_processes(bool multicore, int procs_go_where,
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

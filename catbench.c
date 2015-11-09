#include "cpu_support.h"
#include "log.h"
#include "proc_manip.h"

// Number of times to iterate over all processes when attempting to move them between CPUs
#define REARRANGE_RETRIES 3

static bool rearrange_processes(bool multicore, const cpu_support_t *feats) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	if(multicore) {
		for(int cpu = 0; cpu < feats->num_cores; ++cpu)
			CPU_SET(cpu, &mask);
	} else
		CPU_SET(feats->num_cores - 1, &mask);

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

int main(void) {
	int ret = 0;

	cpu_support_t feats;
	cpu_support(&feats);
	cpu_support_log(LOG_INFO, &feats);

	// TODO: Move after sanity-checks
	if(!rearrange_processes(false, &feats)) {
		log_msg(LOG_FATAL, "Unable to relocate all system processes (permissions?)\n");
		ret = 1;
		goto cleanup;
	}

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

	// TODO: Place process migration here
	// TODO: Run desired benchmarks
	// TODO: Undo process migration

cleanup:
	cpu_support_cleanup(&feats);
	return ret;
}

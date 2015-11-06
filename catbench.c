#include "cpu_support.h"
#include "log.h"

int main(void) {
	int ret = 0;

	cpu_support_t feats;
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

cleanup:
	cpu_support_cleanup(&feats);
	return ret;
}

#include "cpu_support.h"

int main(void) {
	int ret = 0;

	cpu_support_t feats;
	cpu_support(&feats);
	// TODO: Log CPU features
	if(feats.num_cores == 1) {
		ret = 1;
		goto cleanup;
	}
	if(!feats.num_cat_levels) {
		ret = 1;
		goto cleanup;
	}
	cpu_support_foreach_cat_level(cache_level, feats) {
		if (cache_level->cache_level != 3)
			continue;

		if (!cache_level->supported) {
			ret = 1;
			goto cleanup;
		}
		break;
	}

cleanup:
	cpu_support_cleanup(&feats);
	return ret;
}

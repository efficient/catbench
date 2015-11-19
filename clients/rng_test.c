#include "rng.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	if(argc < 4) {
		printf("USAGE: %s <period> <num prime factors> <prime factor>...\n", argv[0]);
		return 1;
	}

	rng_t *lcfc = NULL;
	bool *seen = NULL;
	int ret = 0;
	unsigned period = atoi(argv[1]);
	size_t nfactors = atoi(argv[2]);

	if(nfactors != (unsigned) argc - 3) {
		fprintf(stderr, "<num prime factors> must be trailing args count (%d)\n", argc - 3);
		return 1;
	}

	int pfactors[nfactors];
	for(unsigned idx = 0; idx < nfactors; ++idx)
		pfactors[idx] = atoi(argv[idx + 3]);

	lcfc = rng_lcfc_init(period, nfactors, pfactors);
	if(!lcfc) {
		perror("rng_lcfc_init()");
		ret = 1;
		goto cleanup;
	}

	seen = calloc(period, sizeof *seen);
	if(!seen) {
		perror("calloc()");
		ret = 1;
		goto cleanup;
	}

	for(unsigned iter = 0; iter < period; ++iter) {
		unsigned rn = rng_lcfc(lcfc);
		if(rn >= period) {
			printf("Test FAILED: result %u greater than period!\n", rn);
			ret = 1;
			goto cleanup;
		}
		if(seen[rn]) {
			printf("Test FAILED: %u seen twice within a single period!\n", rn);
			ret = 1;
			goto cleanup;
		}
		seen[rn] = true;
	}
	puts("Test passed.");

cleanup:
	if(seen)
		free(seen);
	if(lcfc)
		rng_lcfc_clean(lcfc);
	return ret;
}

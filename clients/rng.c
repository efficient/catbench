#include "rng.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

struct rng_state {
	unsigned last;
	unsigned coeff;
	unsigned incr;
	unsigned mod;
};

// Picks parameters compliant with Hull-Dobell Thm. to achieve full cycle
rng_t *rng_lcfc_init(unsigned period, size_t nfactors, const int *pfactors) {
	assert(period >= 5);
	assert(nfactors);
	assert(pfactors);

	static bool seeded = false;
	if(!seeded)
		srand(time(NULL));

	rng_t *res = malloc(sizeof *res);
	if(!res)
		return NULL;

	res->last = rand();

	res->coeff = 4;
	for(unsigned idx = 0; idx < nfactors; ++idx)
		res->coeff *= pfactors[idx];
	res->coeff += 1;

	bool coprime;
	do {
		do
			res->incr = rand() % period;
		while(res->incr == 0 || res->incr % period == 1 ||
				res->incr % period == period - 1);

		unsigned iters;
		for(iters = 0; iters < nfactors; ++iters) {
			if(res->incr % pfactors[iters] == 0)
				break;
		}
		coprime = iters == nfactors;
	} while(!coprime);

	res->mod = period;

	return res;
}

void rng_lcfc_clean(rng_t *victim) {
	if(victim)
		free(victim);
}

unsigned rng_lcfc(rng_t *ctxt) {
	return ctxt->last = (ctxt->coeff * ctxt->last + ctxt->incr) % ctxt->mod;
}

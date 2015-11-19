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

static inline bool coprime(unsigned a, size_t bnfacts, const int *bpfacts) {
	unsigned iters;
	for(iters = 0; iters < bnfacts; ++iters) {
		if(a % bpfacts[iters] == 0)
			break;
	}
	return iters == bnfacts;
}

// Picks parameters compliant with Hull-Dobell Thm. to achieve full cycle
rng_t *rng_lcfc_init(unsigned period, size_t nfactors, const int *pfactors) {
	unsigned incr;
	do {
		do
			incr = rand() % period;
		while(incr == 0 || incr % period == 1 || incr % period == period - 1);
	} while(!coprime(incr, nfactors, pfactors));
	return rng_lcfc_init_incr(period, nfactors, pfactors, incr);
}

// Ensures parameters are compliant with Hull-Dobell Thm.
rng_t *rng_lcfc_init_incr(unsigned period, size_t nfactors, const int *pfactors, unsigned incr) {
	assert(period >= 5);
	assert(nfactors);
	assert(pfactors);

	if(!coprime(incr, nfactors, pfactors))
		return NULL;

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

	res->incr = incr;

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

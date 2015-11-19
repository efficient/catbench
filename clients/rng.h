#ifndef RNG_H_
#define RNG_H_

#include <stddef.h>

typedef struct rng_state rng_t;

// pfactors must contain all prime factors of period, which must be at least 5
rng_t *rng_lcfc_init(unsigned period, size_t nfactors, const int *pfactors);

// incr must be coprime with period
rng_t *rng_lcfc_init_incr(unsigned period, size_t nfactors, const int *pfactors, unsigned incr);

void rng_lcfc_clean(rng_t *victim);

unsigned rng_lcfc(rng_t *ctxt);

#endif

#ifndef PREP_SYSTEM_H_
#define PREP_SYSTEM_H_

#include <pqos.h>
#include <stdbool.h>

// Number of times to iterate over all processes when attempting to move them between CPUs
#define REARRANGE_RETRIES 3

bool rearrange_processes(bool multicore, int procs_go_where,
		const struct pqos_cpuinfo *feats);

#endif

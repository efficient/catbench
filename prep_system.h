#ifndef PREP_SYSTEM_H_
#define PREP_SYSTEM_H_

#include <pqos.h>
#include <stdbool.h>

// Number of times to iterate over all processes when attempting to move them between CPUs
#define REARRANGE_RETRIES 3

/* Returns false on error */
bool rearrange_processes(bool multicore, int procs_go_where,
		const struct pqos_cpuinfo *feats);

/* Returns 0 on success
 * Returns -1 on failure */
int prep_system(bool multicore, int procs_go_where);

/* Returns 0 on success
 * Returns -1 on failure */
int cleanup_system(bool unpin_procs);

#endif

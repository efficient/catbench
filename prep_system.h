#ifndef PREP_SYSTEM_H_
#define PREP_SYSTEM_H_

#include <stdbool.h>

// Number of times to iterate over all processes when attempting to move them between CPUs
#define REARRANGE_RETRIES 3

/* Returns 0 on success
 * Returns -1 on failure */
int prep_system(bool multicore, int procs_go_where);

/* Returns 0 on success
 * Returns -1 on failure */
int cleanup_system(bool unpin_procs);

#endif

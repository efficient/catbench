#ifndef DPDK_WRAPPER_H_
#define DPDK_WRAPPER_H_

#include <rte_errno.h>
#include <stdbool.h>
#include <stdio.h>

#define DEFAULT_LEN 1

#define PORT 0

static inline void perr(const char *s, int no) {
	fprintf(stderr, "%s: %s\n", s, rte_strerror(no));
}

static inline void perror_rte(const char *s) {
	perr(s, rte_errno);
}

struct rte_mempool *dpdk_start(int argc, char**argv);

#endif

#ifndef PERF_H_
#define PERF_H_

#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <unistd.h>

static inline int perf_event_open(const struct perf_event_attr *attr, pid_t pid, int cpu,
		int group_fd, unsigned long flags) {
	return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

static inline int exec_v(const char *path, const char *const argv[]) {
	#pragma GCC diagnostic ignored "-Wcast-qual"
	return execv(path, (char *const *) argv);
	#pragma GCC diagnostic pop
}

#endif

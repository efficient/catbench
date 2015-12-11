#ifndef PERF_POLL_H_
#define PERF_POLL_H_

#include <linux/perf_event.h>
#include <stdbool.h>
#include <stddef.h>

int perf_poll_init(size_t count, const struct perf_event_attr *counters);
bool perf_poll_start(int fd);
bool perf_poll_stop(int fd);

#endif

#include "perf_poll.h"

#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "syscallers.h"

int perf_poll_init(size_t count, const struct perf_event_attr *counters) {
	assert(counters);

	int fd = -1;

	for(unsigned idx = 0; idx < count; ++idx) {
		int tfd = perf_event_open(counters + idx, 0, -1, fd, 0);
		if(tfd == -1) {
			int terrno = errno;
			char desc[] = "Initializing perf counter \0\0";
			sprintf(strchr(desc, '\0'), "%u", idx);
			errno = terrno;
			puts(desc);
			return tfd;
		}
		if(fd == -1)
			fd = tfd;
	}

	return fd;
}

bool perf_poll_start(int fd) {
	assert(fd > fileno(stderr));
	return ioctl(fd, PERF_EVENT_IOC_ENABLE) >= 0;
}

bool perf_poll_stop(int fd) {
	assert(fd > fileno(stderr));
	return ioctl(fd, PERF_EVENT_IOC_DISABLE) >= 0;
}

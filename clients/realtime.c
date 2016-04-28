#include "realtime.h"

clock_t realtime(void) {
	struct timespec ns;
	clock_gettime(CLOCK_REALTIME, &ns);
	return ns.tv_sec * 1000000 + ns.tv_nsec / 1000;
}

int comparetimes(const void *first, const void *second) {
	return *(const clock_t *) first - *(const clock_t *) second;
}

#include <sys/mman.h>
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "syscallers.h"

#define MAX_TEST_PROG_CMDLINE_WORDS 5
typedef struct {
	const char *cmdline[MAX_TEST_PROG_CMDLINE_WORDS];
	const char *const null;
} test_prog_t;

static const test_prog_t TEST_PROGS[] = {
	{.cmdline = {"clients/square_evictions", "-n1", "-c0", "-e25", "-r"}},
};
static const int NUM_TEST_PROGS = sizeof TEST_PROGS / sizeof *TEST_PROGS;

#define PERF_BUFFER_NUM_PAGES (1 + (1 << 5))
static const struct perf_event_attr METRIC = {
	.type = PERF_TYPE_HARDWARE,
	.config = PERF_COUNT_HW_CACHE_MISSES,
	.sample_period = 1,
	.sample_type = PERF_SAMPLE_READ | PERF_SAMPLE_TIME | PERF_SAMPLE_CPU,
	.disabled = 0x1,
	.enable_on_exec = 0x1,
	.size = sizeof METRIC,
};

#define SIG_CHILD_PROC_UP  SIGUSR1
#define SIG_EXEC_TEST_PROG SIGUSR1

typedef struct {
	pid_t pid;
	int fd;
	struct perf_event_mmap_page *buf;
} test_proc_t;

static sigset_t block_signal(int signal) {
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, signal);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	return mask;
}

static void await_signal(int signal) {
	int received;
	sigset_t await = block_signal(signal);
	sigwait(&await, &received);
	assert(received == signal);
}

int main(void) {
	test_proc_t children[NUM_TEST_PROGS];
	memset(&children, 1, sizeof children);
	int ret = 0;

	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog) {
		block_signal(SIG_CHILD_PROC_UP);
		pid_t pid = fork();
		switch(pid) {
		case -1:
			perror("Forking child");
			ret = 1;
			goto cleanup;
		case 0:
			kill(getppid(), SIG_CHILD_PROC_UP);
			await_signal(SIG_EXEC_TEST_PROG);
			exec_v(TEST_PROGS[prog].cmdline[0], TEST_PROGS[prog].cmdline);

			perror("Executing test program");
			ret = 1;
			goto cleanup;
		default:
			await_signal(SIG_CHILD_PROC_UP);

			children[prog].pid = pid;

			struct perf_event_attr metric;
			memcpy(&metric, &METRIC, sizeof metric);
			int fd = perf_event_open(&metric, pid, -1, -1, 0);
			if(fd < 0) {
				fputs("Starting Perf: permissions?\n", stderr);
				ret = 1;
				goto cleanup;
			}
			children[prog].fd = fd;

			struct perf_event_attr instrs = {
				.type = PERF_TYPE_HARDWARE,
				.config = PERF_COUNT_HW_INSTRUCTIONS,
				.size = sizeof instrs,
			};
			perf_event_open(&instrs, pid, -1, fd, PERF_FLAG_FD_OUTPUT);

			struct perf_event_mmap_page *buf =
					mmap(NULL, PERF_BUFFER_NUM_PAGES * getpagesize(),
					PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if(!buf || buf == MAP_FAILED) {
				perror("Mapping perf buffer");
				ret = 1;
				goto cleanup;
			}
			children[prog].buf = buf;

			printf("Data buffer at %llu, size %llu\n", buf->data_offset, buf->data_size);
			printf("Data head at %llu\n", buf->data_head);
		}
	}

	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog)
		if(kill(children[prog].pid, SIG_EXEC_TEST_PROG)) {
			ret = 1;
			goto cleanup;
		}

	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog) {
		if(waitpid(children[prog].pid, NULL, 0) < 0)
			perror("Awaiting child");

		printf("Test program ran for %llu ns\n", children[prog].buf->time_running);
		printf("Data head at %llu\n", children[prog].buf->data_head);
	}

cleanup:
	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog)
		if(children[prog].buf)
			munmap(children[prog].buf, PERF_BUFFER_NUM_PAGES * getpagesize());
	return ret;
}

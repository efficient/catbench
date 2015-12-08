#include <sys/wait.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "syscallers.h"

#define MAX_TEST_PROG_CMDLINE_WORDS 5
typedef struct {
	const char *cmdline[MAX_TEST_PROG_CMDLINE_WORDS];
	const char *const null;
} test_prog_t;

static const test_prog_t TEST_PROGS[] = {
	{.cmdline = {"clients/square_evictions", "-n1", "-c0", "-e200", "-r"}},
};
static const int NUM_TEST_PROGS = sizeof TEST_PROGS / sizeof *TEST_PROGS;

#define SIG_CHILD_PROC_UP  SIGUSR1
#define SIG_EXEC_TEST_PROG SIGUSR1

typedef struct {
	pid_t pid;
} test_proc_t;

static inline void await_signal(int signal) {
	int received;
	sigset_t await;
	sigemptyset(&await);
	sigaddset(&await, signal);
	sigprocmask(SIG_BLOCK, &await, NULL);
	sigwait(&await, &received);
	assert(received == signal);
}

int main(void) {
	test_proc_t children[NUM_TEST_PROGS];
	int ret = 0;

	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog) {
		pid_t pid = fork();
		switch(pid) {
		case -1:
			perror("Forking child");
			ret = 1;
			goto cleanup;
		case 0:
			await_signal(SIG_EXEC_TEST_PROG);
			exec_v(TEST_PROGS[prog].cmdline[0], TEST_PROGS[prog].cmdline);

			perror("Executing test program");
			ret = 1;
			goto cleanup;
		default:
			children[prog].pid = pid;
		}
		sleep(1);
	}

	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog)
		if(kill(children[prog].pid, SIG_EXEC_TEST_PROG)) {
			perror("Signalling child");
			ret = 1;
			goto cleanup;
		}

	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog)
		if(waitpid(children[prog].pid, NULL, 0) < 0)
			perror("Awaiting child");

cleanup:
	return ret;
}

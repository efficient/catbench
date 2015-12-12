#include "bench_commands.h"

#include <sys/wait.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "syscallers.h"

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

int parse_common_args(int argc, char** argv, args_t* args) {
	int ret = 0;
	// Get dem opts
	int c, has_p = 0;
	while ((c = getopt(argc, argv, "hqup:")) != -1) {
		switch (c) {
			case 'u':
				args->unpin_procs = 1;
				break;
			case 'p':
				args->procs_go_where = atoi(optarg);
				has_p = 1;
				break;
			case 'q':
				args->quit = 1;
				break;
			case 'h':
				log_msg(LOG_INFO, 
						"Usage: \n"
						"  -p CORE: Pin system processes to CORE\n"
						"  -u: Unpin all system processes\n"
						"  -q: Quit after migrating, before benchmarks\n"
						"  -h: Print this message and exit\n");
				return 0;
		}
	}

	if (has_p && args->unpin_procs) {
		log_msg(LOG_FATAL, "Cannot specify both -p and -u");
	}
	return ret;
}

bool run_benchmarks(const test_prog_t* test_progs, const int num_test_progs) {
	log_msg(LOG_INFO, "On your mark\n");
	test_proc_t children[num_test_progs];
	memset(&children, 1, sizeof children);
	bool ret = true;

	for(int prog = 0; prog < num_test_progs; ++prog) {
		block_signal(SIG_CHILD_PROC_UP);
		pid_t pid = fork();
		switch(pid) {
			case -1:
				perror("Forking child");
				ret = false;
				goto cleanup;
			case 0:
				// TODO: Set processor affinity
				kill(getppid(), SIG_CHILD_PROC_UP);
				await_signal(SIG_EXEC_TEST_PROG);
				exec_v(test_progs[prog].cmdline[0], test_progs[prog].cmdline);

				perror("Executing test program");
				ret = false;
				goto cleanup;
			default:
				await_signal(SIG_CHILD_PROC_UP);

				children[prog].pid = pid;

				// TODO: Initialize performance counters
		}
	}

	for(int prog = 0; prog < num_test_progs; ++prog)
		if(kill(children[prog].pid, SIG_EXEC_TEST_PROG)) {
			ret = false;
			goto cleanup;
		}

	for(int prog = 0; prog < num_test_progs; ++prog) {
		if(waitpid(children[prog].pid, NULL, 0) < 0)
			perror("Awaiting child");

		// TODO: Collect and output results.
	}

cleanup:
	return ret;
}

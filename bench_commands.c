#include "bench_commands.h"

#include <sys/sysinfo.h>
#include <sys/wait.h>

#include <assert.h>
#include <sched.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "syscallers.h"

// TODO better solution for this
#define NUM_CPUS 8

#define SELF_PID 0

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

static test_proc_t* childpids = NULL;
static int num_childpids = 0;

bool run_benchmarks(const test_prog_t* test_progs, const int num_test_progs) {
	log_msg(LOG_INFO, "On your mark\n");
	bool ret = true;
	childpids = malloc(num_test_progs * sizeof(*childpids));
	if(childpids == NULL) {
		perror("malloc");
		/* If this happens we're pretty screwed and probably dead anyways, not much need to go to cleanup */
		goto cleanup;
	}
	num_childpids = num_test_progs;
	memset(childpids, 1, sizeof *childpids);

	for(int prog = 0; prog < num_test_progs; ++prog) {
		block_signal(SIG_CHILD_PROC_UP);
		pid_t pid = fork();
		switch(pid) {
			case -1:
				perror("Forking child");
				ret = false;
				goto cleanup;
			case 0: {
				cpu_set_t* cpuset;
				int num_cpus = get_nprocs_conf();
				cpuset = CPU_ALLOC(num_cpus);
				if(cpuset == NULL) {
					log_msg(LOG_FATAL, "CPU_ALLOC for setaffinity failed");
					ret = false;
					goto cleanup;
				}
				int size = CPU_ALLOC_SIZE(num_cpus);
				if(size <= 0) {
					ret = false;
					log_msg(LOG_FATAL, "CPU_ALLOC_SIZE failed\n");
					goto cleanup;
				}
				if(test_progs[prog].target_cpu >= num_cpus) {
					ret = false;
					log_msg(LOG_FATAL, "Invalid CPU number\n");
					goto cleanup;
				}
				CPU_ZERO_S(size, cpuset);
				CPU_SET_S(test_progs[prog].target_cpu, size, cpuset);
				int err = sched_setaffinity(SELF_PID, size, cpuset);
				if(err == -1) {
					perror("sched_setaffinity");
					ret = false;
					goto cleanup;
				}
				CPU_FREE(cpuset);
				kill(getppid(), SIG_CHILD_PROC_UP);
				await_signal(SIG_EXEC_TEST_PROG);
				exec_v(test_progs[prog].cmdline[0], test_progs[prog].cmdline);

				perror("Executing test program");
				ret = false;
				goto cleanup;
			}
			default:
				await_signal(SIG_CHILD_PROC_UP);

				childpids[prog].pid = pid;

				// TODO: Initialize performance counters
				// No longer necessary because this function returns after all children start (?)
		}
	}

	for(int prog = 0; prog < num_test_progs; ++prog)
		if(kill(childpids[prog].pid, SIG_EXEC_TEST_PROG)) {
			ret = false;
			goto cleanup;
		}

cleanup:
	return ret;
}

/* TODO: Error checking and failure behavior seems incorrect here */
int wait_benchmarks() {
	int ret = 0;
	if(childpids == NULL)
		return 0;
	for(int prog = 0; prog < num_childpids; ++prog) {
		if(waitpid(childpids[prog].pid, NULL, 0) < 0) {
			perror("Awaiting child");
			ret = -1;
		}
		// TODO: Collect and output results. <-- Does this go in this function or in main?
	}
	childpids = NULL;
	return ret;
}

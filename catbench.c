#include <sys/wait.h>
#include <assert.h>
#include <pqos.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "prep_system.h"
#include "proc_manip.h"
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

#define SIG_CHILD_PROC_UP  SIGUSR1
#define SIG_EXEC_TEST_PROG SIGUSR1

typedef struct {
	pid_t pid;
	// TODO: Store per-process performance data handles here.
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

static bool run_benchmarks(const struct pqos_cpuinfo *feats) {
	(void) feats;
	log_msg(LOG_INFO, "On your mark\n");
	test_proc_t children[NUM_TEST_PROGS];
	memset(&children, 1, sizeof children);
	bool ret = true;

	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog) {
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
				exec_v(TEST_PROGS[prog].cmdline[0], TEST_PROGS[prog].cmdline);

				perror("Executing test program");
				ret = false;
				goto cleanup;
			default:
				await_signal(SIG_CHILD_PROC_UP);

				children[prog].pid = pid;

				// TODO: Initialize performance counters
		}
	}

	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog)
		if(kill(children[prog].pid, SIG_EXEC_TEST_PROG)) {
			ret = false;
			goto cleanup;
		}

	for(int prog = 0; prog < NUM_TEST_PROGS; ++prog) {
		if(waitpid(children[prog].pid, NULL, 0) < 0)
			perror("Awaiting child");

		// TODO: Collect and output results.
	}

cleanup:
	return ret;
}

int main(int argc, char** argv) {
	int ret = 0;
	int unpin_procs = 0;
	int procs_go_where = 0;
	int quit = 0;

	// Get dem opts
	int c, has_p = 0;
	while ((c = getopt(argc, argv, "hqup:")) != -1) {
		switch (c) {
			case 'u':
				unpin_procs = 1;
				break;
			case 'p':
				procs_go_where = atoi(optarg);
				has_p = 1;
				break;
			case 'q':
				quit = 1;
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

	if (has_p && unpin_procs) {
		log_msg(LOG_FATAL, "Cannot specify both -p and -u");
	}

	struct pqos_config cfg = {
		.fd_log = log_get_dest(),
		.verbose = log_get_verbosity(),
		.topology = NULL,
		.free_in_use_rmid = 0,
		.cdp_cfg = PQOS_REQUIRE_CDP_OFF,
	};
	int pqos_res = pqos_init(&cfg);
	if(pqos_res != PQOS_RETVAL_OK) {
		log_msg(LOG_FATAL, "Library init failed with code %d (HW support?)\n", pqos_res);
		ret = 1;
		goto cleanup;
	}

	const struct pqos_cpuinfo *traits = NULL;
	const struct pqos_cap *feats = NULL;
	pqos_cap_get(&feats, &traits);

	if(!rearrange_processes(unpin_procs, procs_go_where, traits)) {
		log_msg(LOG_FATAL, "Unable to relocate all system processes (permissions?)\n");
		// Probably don't want to quit early here, because failing to migrate is common
		// ret = 1;
		// goto cleanup;
	}
	if (quit) goto cleanup;
	run_benchmarks(traits);
	if(!unpin_procs)
		rearrange_processes(true, 0, traits);

cleanup:
	pqos_fini();
	return ret;
}

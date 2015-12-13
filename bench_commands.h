#ifndef BENCH_COMMANDS_H_
#define BENCH_COMMANDS_H_

#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>

#define MAX_TEST_PROG_CMDLINE_WORDS 8

typedef struct {
	int unpin_procs;
	int procs_go_where;
	int quit;
} args_t;

typedef struct {
	const char *cmdline[MAX_TEST_PROG_CMDLINE_WORDS];
	const char *const null;
	const int target_cpu;
} test_prog_t;

#undef MAX_TEST_PROG_CMDLINE_WORDS

#define SIG_CHILD_PROC_UP  SIGUSR1
#define SIG_EXEC_TEST_PROG SIGUSR1

typedef struct {
	pid_t pid;
	// TODO: Store per-process performance data handles here.
} test_proc_t;

bool run_benchmarks(const test_prog_t* test_progs, const int num_test_progs);

/* If wait_benchmarks is run without first running run_benchmarks, wait_benchmark will return immediately with return value 0 */
int wait_benchmarks(void);

// Upon any error will return a negative value
// Upon success returns 0
int parse_common_args(int argc, char** argv, args_t* args);

#endif

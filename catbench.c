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

#include "bench_commands.h"
#include "log.h"
#include "prep_system.h"
#include "proc_manip.h"
#include "syscallers.h"

static const test_prog_t test_progs[] = {
	{.cmdline = {"clients/square_evictions", "-n1", "-c0", "-e25", "-r"}},
};

static const int num_test_progs = sizeof test_progs / sizeof *test_progs;

int main(int argc, char** argv) {
	int ret = 0;
	args_t args;
	parse_common_args(argc, argv, &args);
	int unpin_procs = args.unpin_procs;
	int procs_go_where = args.procs_go_where;
	int quit = args.quit;
	ret = prep_system(unpin_procs, procs_go_where);
	if(ret < 0)
		return ret;
	if (quit)
		return ret;
	run_benchmarks(test_progs, num_test_progs);
	ret = cleanup_system(unpin_procs);
	return ret;
}

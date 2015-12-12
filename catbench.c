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

int main(int argc, char** argv) {
	int ret = 0;
	args_t args;
	parse_args(argc, argv, &args);
	int unpin_procs = args.unpin_procs;
	int procs_go_where = args.procs_go_where;
	int quit = args.quit;
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
		goto end;
	}

	const struct pqos_cpuinfo *traits = NULL;
	const struct pqos_cap *feats = NULL;
	pqos_cap_get(&feats, &traits);
	prep_system(unpin_procs, procs_go_where, traits);
	if (quit) goto end;
	run_benchmarks();
	ret = cleanup_system(unpin_procs, traits);
end:
	return ret;
}

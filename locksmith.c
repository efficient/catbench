#include <assert.h>
#include <pqos.h>
#include <string.h>
#include <stdio.h>

#include "bench_commands.h"
#include "log.h"
#include "prep_system.h"
#define LARGE_NUMBER "100"

#define COMMON_COS 	1
#define OTHER_COS 	2

#define READER_CORE 	0
#define SWITCHER_CORE	1

#define NUM_SWITCHES	1000

// TODO add flag to square_evictions for timing each memory access
static test_prog_t progs[] = {
	{
		.cmdline = {"clients/square_evictions", "-e100", "-c100", "-n"LARGE_NUMBER, "-m"},
		.target_cpu = 0,
	},
};

static const unsigned COS_MAPPINGS[] = {
	[READER_CORE] = COMMON_COS,
	[SWITCHER_CORE] = COMMON_COS
};
static const size_t NUM_COS_MAPPINGS = sizeof COS_MAPPINGS / sizeof *COS_MAPPINGS;

static const struct pqos_l3ca COS_MASKS[] = {
	{
		.class_id = COMMON_COS,
		.ways_mask = 0x1 | 0x2 | 0x4 | 0x8,
	}, {
		.class_id = OTHER_COS,
		.ways_mask = 0x10 | 0x20 | 0x40 | 0x80,
	},
};
static const size_t NUM_COS_MASKS = sizeof COS_MASKS / sizeof *COS_MASKS;

int main(int argc, char** argv) {
	(void) argc;
	(void) argv;
	if(prep_system(false, -1) < 0) {
		log_msg(LOG_FATAL, "Failed to complete initial setup\n");
		return 1;
	}
	if(pqos_l3ca_set(0, NUM_COS_MASKS, COS_MASKS) != PQOS_RETVAL_OK) {
		log_msg(LOG_FATAL, "Failed to set CoS masks\n");
		return 1;
	}
	/* Librarify this? */
	for(unsigned core = 0; core < NUM_COS_MAPPINGS; ++core) {
		if(pqos_l3ca_assoc_set(core, COS_MAPPINGS[core]) != PQOS_RETVAL_OK) {
			log_msg(LOG_FATAL, "Failed to associate CoS with core\n");
			return 1;
		}
		unsigned thenum = 0;
		assert(pqos_l3ca_assoc_get(core, &thenum) == PQOS_RETVAL_OK);
		assert(thenum == COS_MAPPINGS[core]);
		// FIXME: This is needed due to a race in libpqos: otherwise, only the first works!
		sleep(2);
	}
	run_benchmarks(progs, 1);
	for(int i = 0; i < NUM_SWITCHES; ++i) {
		if(pqos_l3ca_assoc_set(SWITCHER_CORE, OTHER_COS) != PQOS_RETVAL_OK) {
			log_msg(LOG_FATAL, "Failed to switch Cos on switcher process\n");
		}
	}
	/* Keep swapping COS of our other core */
	// TODO fix this
	wait_benchmarks();
	cleanup_system(true);
}

#include <assert.h>
#include <pqos.h>
#include <string.h>
#include <stdio.h>

#include "bench_commands.h"
#include "log.h"
#include "prep_system.h"

// Should be at least 2, on 1 it might measure memory fetches, 2+ will use definitely use cache
#define LARGE_NUMBER "2"

#define COMMON_COS 	1
#define OTHER_COS 	2

#define READER_CORE 	0
#define SWITCHER_CORE	1

// 21 takes about 8 seconds
// 23 takes about 40 seconds
// 24 takes about 1 minute
#define NUM_SWITCHES	(1 << 24)

#define MASK_SWAP
#undef COS_SWAP

static test_prog_t progs[] = {
	{
		.cmdline = {"clients/square_evictions", "-e25", "-c25", "-n"LARGE_NUMBER, "-m", "-h"},
		.target_cpu = 0,
	},
};

// Initial state is both cores on same CoS
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

static const struct pqos_l3ca COS_MASKS2[] = {
	{
		.class_id = OTHER_COS,
		.ways_mask = 0x1 | 0x2 | 0x4 | 0x8,
	},
};

static const struct pqos_l3ca COS_MASKS3[] = {
	{
		.class_id = OTHER_COS,
		.ways_mask = 0x10 | 0x20 | 0x40 | 0x80,
	},
};

static const size_t NUM_COS_MASKS = sizeof COS_MASKS / sizeof *COS_MASKS;

int main(int argc, char** argv) {
	int swappy = 0;
	/* lol */
	if(argc > 1) {
		if(argv[1][0] == '-' && argv[1][1] == 's')
			swappy = 1;
	}
	/* end lol */
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
	if(swappy) {
#ifdef  COS_SWAP
		printf("Swapping CoSes\n");
		for(int i = 0; i < NUM_SWITCHES; ++i) {
			if(pqos_l3ca_assoc_set(SWITCHER_CORE, OTHER_COS) != PQOS_RETVAL_OK) {
				log_msg(LOG_FATAL, "Failed to switch Cos on switcher process\n");
			}
		}
		/* Keep swapping COS of our other core */
		// TODO fix this
		printf("Done swapping!\n");
#endif
#ifdef MASK_SWAP
		printf("Swapping masks\n");
		for(int i = 0; i < NUM_SWITCHES; ++i) {
			pqos_l3ca_set(0, 1, COS_MASKS2);
			pqos_l3ca_set(0, 1, COS_MASKS3);
		}
		printf("Done swapping!\n");

#endif
	}
	wait_benchmarks();
	cleanup_system(true);
}

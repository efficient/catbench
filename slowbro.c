#include <pqos.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "bench_commands.h"
#include "log.h"
#include "prep_system.h"

#define ARG_IDX_TO_REPL_WITH_LOG_FLAG 7
static test_prog_t progs[] = {
	{
		.cmdline = {"clients/square_evictions", "-e100", "-c0", "-n1", "-p10", "-hr"},
		.target_cpu = 0,
	}, {
		.cmdline = {"clients/square_evictions", "-e100", "-c0", "-n1", "-p4", "-hr", "-j480", NULL},
		.target_cpu = 1,
	}, {
		.cmdline = {"clients/square_evictions", "-e100", "-c0", "-n1", "-p4", "-hr", "-j480", NULL},
		.target_cpu = 2,
	},
};
static const size_t NUM_PROGS = sizeof progs / sizeof *progs;

#define COS_MAPPING_SWITCH_WHICH_CORE 0
#define COS_MAPPING_SWITCH_TO_COS     2
static const struct pqos_l3ca COS_MASK_REPLACEMENT = {
	.class_id = 3,
	.ways_mask = 0x2,
};

static const unsigned COS_MAPPINGS[] = {
	[0] = 3,
	[1] = 1,
	[2] = 2,
};
static const size_t NUM_COS_MAPPINGS = sizeof COS_MAPPINGS / sizeof *COS_MAPPINGS;

static const struct pqos_l3ca COS_MASKS[] = {
	{
		.class_id = 3,
		.ways_mask = 0x1,
	}, {
		.class_id = 1,
		.ways_mask = 0x1,
	}, {
		.class_id = 2,
		.ways_mask = 0x2,
	}, {
		.class_id = 0,
		.ways_mask = 0xffc,
	}
};
static const size_t NUM_COS_MASKS = sizeof COS_MASKS / sizeof *COS_MASKS;

static inline clock_t realclock(void) {
	struct timespec ns;
	clock_gettime(CLOCK_REALTIME, &ns);
	return ns.tv_sec * 1000000 + ns.tv_nsec / 1000;
}

int main(int argc, char **argv) {
	bool slowpoke = true;

	if(argc < 3) {
		printf("USAGE: %s <outfile1> <outfile2> []\n", argv[0]);
		return 1;
	}
	if(argc >= 4) {
		puts("OPERATING IN SLOWPOKE MODE... VERY MASK, SUCH BITWISE, SO DIFFERENT");
		slowpoke = true;
	}

	char logswitch1[2 + strlen(argv[1]) + 1];
	sprintf(logswitch1, "-l%s", argv[1]);
	progs[1].cmdline[ARG_IDX_TO_REPL_WITH_LOG_FLAG] = logswitch1;

	char logswitch2[2 + strlen(argv[2]) + 1];
	sprintf(logswitch2, "-l%s", argv[2]);
	progs[2].cmdline[ARG_IDX_TO_REPL_WITH_LOG_FLAG] = logswitch2;

	if(prep_system(false, -1) < 0) {
		log_msg(LOG_FATAL, "Performing initial setup\n");
		return 1;
	}
	if(pqos_l3ca_set(0, NUM_COS_MASKS, COS_MASKS) != PQOS_RETVAL_OK) {
		log_msg(LOG_FATAL, "Failed to set CoS masks\n");
		return 1;
	}
	for(unsigned core = 0; core < NUM_COS_MAPPINGS; ++core) {
		if(pqos_l3ca_assoc_set(core, COS_MAPPINGS[core]) != PQOS_RETVAL_OK) {
			log_msg(LOG_FATAL, "Failed to associate CoS with core\n");
			return 1;
		}
	}

	run_benchmarks(progs, NUM_PROGS);

	usleep(4500);
	printf("About to change CoS: %ld\n", realclock());
	if(!slowpoke) {
		if(pqos_l3ca_assoc_set(COS_MAPPING_SWITCH_WHICH_CORE, COS_MAPPING_SWITCH_TO_COS) !=
				PQOS_RETVAL_OK) {
			log_msg(LOG_FATAL, "Failed to reassociate mobile test program\n");
			return 1;
		}
	} else {
		if(pqos_l3ca_set(0, 1, &COS_MASK_REPLACEMENT) != PQOS_RETVAL_OK) {
			log_msg(LOG_FATAL, "Failed to change mask of mobile test program\n");
			return 1;
		}
	}
	printf("Just changed CoS: %ld\n", realclock());

	wait_benchmarks();
	cleanup_system(true);

	return 0;
}

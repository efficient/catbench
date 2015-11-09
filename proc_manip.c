#include "log.h"
#include "proc_manip.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline bool trunc_newline(char *victim) {
	char *newline = strchr(victim, '\n');
	if(newline)
		*newline = '\0';
	return newline;
}

int proc_manip_rebalance_system(uint8_t *errret, const cpu_set_t *cores) {
	FILE *pids = popen("/bin/ps -eo pid=", "r");
	char *line = NULL;
	int chrcnt;
	int updated = 0;
	size_t bufsiz = 0;
	uint8_t err = 0x0;

	while(errno = 0, (chrcnt = getline(&line, &bufsiz, pids)) >= 0) {
		if(!chrcnt)
			continue;

		char *endparse = NULL;

		errno = 0;
		pid_t id = strtol(line, &endparse, 10);

		int errnot = errno;
		bool stationary = endparse == line;

		trunc_newline(line);
		if(stationary) {
			log_msg(LOG_WARNING, "No characters parsed in PID '%s'\n", line);
			err |= PROC_MANIP_ERR_PARSE_PID;
			continue;
		} else if(errnot) {
			log_msg(LOG_WARNING, "%s when parsing PID '%s'\n", strerror(errnot), line);
			err |= PROC_MANIP_ERR_PARSE_PID;
			continue;
		}

		cpu_set_t curr_cores;
		if(sched_getaffinity(id, sizeof curr_cores, &curr_cores)) {
			log_msg(LOG_WARNING, "Unable to read current affinity of process %d\n", id);
			err |= PROC_MANIP_ERR_READ_MASK;
		}

		if(CPU_EQUAL(cores, &curr_cores)) {
			log_msg(LOG_VERBOSE, "Process %d has already been migrated\n", id);
			continue;
		}

		errno = 0;
		sched_setaffinity(id, sizeof *cores, cores);
		switch(errno) {
		case EFAULT:
			assert(false);
			err |= PROC_MANIP_ERR_UNK_FAULT;
			goto cleanup;
		case EINVAL:
			log_msg(LOG_WARNING, "Unable to migrate pid %d between cores\n", id);
			continue;
		case EPERM:
			log_msg(LOG_ERROR, "Access denied on sched_setaffinity() attempt\n");
			err |= PROC_MANIP_ERR_INSF_PRIV;
			goto cleanup;
		case ESRCH:
			log_msg(LOG_VERBOSE, "Process %d finished before affinity change\n", id);
			continue;
		}
		log_msg(LOG_INFO, "Successfully migrated process %d between cores\n", id);
		++updated;
	}
	assert(!errno);

cleanup:
	free(line);
	pclose(pids);

	*errret = err;
	return updated;
}

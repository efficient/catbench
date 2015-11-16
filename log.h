#ifndef CATBENCH_LOG_H_
#define CATBENCH_LOG_H_

#include <stdio.h>

typedef enum {
	LOG_FATAL,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_VERBOSE,
} log_verbosity_t;

log_verbosity_t log_get_verbosity(void);
void log_set_verbosity(log_verbosity_t max);
FILE *log_get_dest(void);
void log_set_dest(FILE *stream);
int log_msg(log_verbosity_t verb, const char *format, ...);

#endif

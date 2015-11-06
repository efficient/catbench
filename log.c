#include "log.h"

#include <stdarg.h>

static const char *const VERBOSITY_DESCRS[] = {
	"FATAL: ",
	"ERROR: ",
	"WARNING: ",
	"INFO: ",
	"VERBOSE: ",
};

static log_verbosity_t max_handled = LOG_VERBOSE;
static FILE *write_dest = NULL;

void log_set_verbosity(log_verbosity_t max) {
	max_handled = max;
}

void log_set_dest(FILE *stream) {
	write_dest = stream;
}

int log_msg(log_verbosity_t verb, const char *format, ...) {
	if (!write_dest)
		write_dest = stderr;

	if(verb > max_handled)
		return 0;

	va_list ap;
	va_start(ap, format);
	fputs(VERBOSITY_DESCRS[verb], write_dest);
	int ret = vfprintf(write_dest, format, ap);
	va_end(ap);

	return ret;
}

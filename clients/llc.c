#include "llc.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char SYS_CACHE_PREFIX[] =            "/sys/devices/system/cpu/cpu0/cache/";
static const char SYS_CACHE_SUFFIX_line_size[] =  "/coherency_line_size";
static const char SYS_CACHE_SUFFIX_num_sets[] =   "/number_of_sets";
static const char SYS_CACHE_SUFFIX_assoc_ways[] = "/ways_of_associativity";

static char *basepath = NULL;
static size_t basepath_len = 0;

static int filter_prefix_index(const struct dirent *candidate) {
	const char prefix[] = "index";
	return !strncmp(candidate->d_name, prefix, sizeof prefix - 1);
}

static inline bool strncat_expand(char **dest, const char *src, size_t *n) {
	size_t addllen = strlen(src);
	*n += addllen;
	*dest = realloc(*dest, *n + 1);
	if(!*dest)
		return false;
	strncat(*dest, src, addllen);
	return true;
}

static inline int llc_accessor_helper(const char *suffix) {
	char *path = NULL;
	FILE *file = NULL;
	int ret;
	bool nonreent = false;

	if(!basepath) {
		nonreent = true;
		if(!llc_init()) {
			ret = LLC_ERROR;
			goto cleanup;
		}
	}

	path = strdup(basepath);
	if(!path) {
		perror("Duplicating shared base path");
		ret = LLC_ERROR;
		goto cleanup;
	}

	if(!strncat_expand(&path, suffix, &basepath_len)) {
		perror("Concatenating onto copy of base path");
		ret = LLC_ERROR;
		goto cleanup;
	}

	file = fopen(path, "r");
	if(!file) {
		perror("Opening sysfs cache file");
		ret = LLC_ERROR;
		goto cleanup;
	}

	if(fscanf(file, "%d", &ret) != 1) {
		perror("Parsing sysfs cache file contents");
		ret = LLC_ERROR;
		goto cleanup;
	}

cleanup:
	if(file)
		fclose(file);
	if(path)
		free(path);
	if(nonreent)
		llc_cleanup();
	return ret;
}

#define GEN_LLC_ACCESSOR(property) \
	int llc_##property(void) { \
		return llc_accessor_helper(SYS_CACHE_SUFFIX_##property); \
	}

bool llc_init(void) {
	struct dirent **subdirs = NULL;
	bool ret = true;

	int numdirs = scandir(SYS_CACHE_PREFIX , &subdirs, filter_prefix_index, alphasort);
	if(numdirs < 0) {
		perror("Scanning sysfs cpu0");
		ret = false;
		goto cleanup;
	} else if(numdirs == 0) {
		fprintf(stderr, "No cache levels for CPU 0\n");
		goto cleanup;
	}

	basepath = strdup(SYS_CACHE_PREFIX);
	if(!basepath) {
		perror("Duplicating string constant");
		ret = false;
		goto cleanup;
	}
	basepath_len = sizeof SYS_CACHE_PREFIX - 1;

	const char *llc_index = subdirs[numdirs - 1]->d_name;
	if(!strncat_expand(&basepath, llc_index, &basepath_len)) {
		perror("Concatenating on cache level index");
		ret = false;
		goto cleanup;
	}

cleanup:
	if(subdirs) {
		for(int index = 0; index < numdirs; ++index)
			free(subdirs[index]);
		free(subdirs);
	}
	return ret;
}

GEN_LLC_ACCESSOR(line_size)
GEN_LLC_ACCESSOR(num_sets)
GEN_LLC_ACCESSOR(assoc_ways)

void llc_cleanup(void) {
	if(basepath) {
		free(basepath);
		basepath = NULL;
		basepath_len = 0;
	}
}

#include <sys/mman.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_LEN 1

typedef struct {
	int len;
} args_t;

static void ptrchase_setup(uintptr_t *arr, int len) {
	if(len == 1) {
		*arr = (uintptr_t) arr;
		return;
	}

#ifndef NDEBUG
	for(int idx = 0; idx < len; ++idx)
		assert(!arr[idx]);
#endif

	srand(time(NULL));
	uintptr_t *curr = arr;
	for(int count = 1; count < len; ++count) {
		int idx = curr - arr;
		int tgt;
		while((tgt = rand() % len) == idx || arr[tgt]);
		*curr = (uintptr_t) (arr + tgt);
		curr = arr + tgt;
	}
	*curr = (uintptr_t) arr;

#ifndef NDEBUG
	for(int idx = 0; idx < len; ++idx) {
		assert(!(arr[idx] & 0x1));
		assert(arr[idx] >= (uintptr_t) arr);
		assert(arr[idx] < (uintptr_t) (arr + len));
		assert(arr[idx] != (uintptr_t) (arr + idx));
	}

	uintptr_t *cur = arr;
	for(int count = 1; count < len; ++count) {
		cur = (uintptr_t *) *cur;
		assert(cur != arr);
	}
	assert(*cur == (uintptr_t) arr);
#endif
}

static int experiment(args_t *args) {
	uintptr_t *arr = mmap(0, args->len * sizeof(*arr), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE, -1, 0);
	if(!arr || arr == MAP_FAILED) {
		perror("Allocating array");
		return 1;
	}
	ptrchase_setup(arr, args->len);

	uintptr_t *cur = arr;
	while((cur = (uintptr_t *) *cur) != arr);

	munmap(arr, args->len);
	return 0;
}

static bool parse_arg_arg(int *dest, char flag, int min) {
	int val;

	if(sscanf(optarg, "%d", &val) != 1) {
		fprintf(stderr, "%c: Unexpected subargument '%s'\n", flag, optarg);
		return false;
	} else if(val < min) {
		fprintf(stderr, "%c: Expected gte %d but got '%d'\n", flag, min, val);
		return false;
	}

	*dest = val;
	return true;
}

int main(int argc, char **argv) {
	args_t args = {
		.len = DEFAULT_LEN,
	};

	int each_arg;
	while((each_arg = getopt(argc, argv, "l:")) != -1)
		switch(each_arg) {
		case 'l':
			if(!parse_arg_arg(&args.len, 'l', 1))
				return 127;
			break;
		default:
			printf("USAGE: %s [-l #]\n", argv[0]);
			printf(
					" -l #: LENGTH to traverse (octawords, default %d)\n",
					DEFAULT_LEN);
			return 127;
		}

	return experiment(&args);
}

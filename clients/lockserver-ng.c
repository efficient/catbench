#include <sys/mman.h>
#include <assert.h>
#include <rte_ethdev.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "dpdk_wrapper.h"
#include "realtime.h"

//#define AVE_MEDIAN
#define TIMING_BUFFER_LEN 5000
#define WARMUP            600

typedef struct {
	int len;
} args_t;

static bool loop = true;
static int iter = 0;
#ifdef AVE_MEDIAN
static clock_t times[TIMING_BUFFER_LEN];
#else
static double ave;
#endif

static void sigterm_handler(int signal) {
	(void) signal;
	iter -= WARMUP;
	if(iter) {
#ifdef AVE_MEDIAN
		qsort(times, iter, sizeof *times, comparetimes);
		double ave = times[iter / 2];
		if(iter % 2 == 0) {
			ave += times[iter / 2 - 1];
			ave /= 2;
		}
#else
		ave /= iter;
#endif
		printf("Average: %f us\n", ave);
	}
	exit(!iter);
}

static void sigint_handler(int signal) {
	(void) signal;
	loop = false;
}

static void paddr(struct ether_addr *mac) {
	printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
			mac->addr_bytes[0], mac->addr_bytes[1], mac->addr_bytes[2],
			mac->addr_bytes[3], mac->addr_bytes[4], mac->addr_bytes[5]);
}

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
	int ret = 0;
	uintptr_t *arr = mmap(0, args->len * sizeof(*arr), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE, -1, 0);
	if(!arr || arr == MAP_FAILED) {
		perror("Allocating array");
		return 1;
	}
	puts("Setting up pointers for chasing...");
	ptrchase_setup(arr, args->len);

	struct sigaction sigterm = {.sa_handler = sigterm_handler};
	struct sigaction sigint = {.sa_handler = sigint_handler};
	if(sigaction(SIGTERM, &sigterm, NULL) || sigaction(SIGINT, &sigint, NULL)) {
		perror("Installing sigint handler");
		ret = 1;
		goto cleanup;
	}

	struct ether_addr laddr;
	rte_eth_macaddr_get(PORT, &laddr);
	printf("Listening on MAC: ");
	paddr(&laddr);

	while(loop) {
		clock_t thistime;

		assert(iter <= TIMING_BUFFER_LEN + 1);
		struct rte_mbuf *packet = NULL;
		while(true) {
			if(!loop)
				goto stoploop;
			uint16_t ct = rte_eth_rx_burst(PORT, 0, &packet, 1);
			if(ct) {
				struct ether_hdr *eth = rte_pktmbuf_mtod(packet, struct ether_hdr *);
				bool addressed_to_me = false;
				addressed_to_me = is_same_ether_addr(&eth->d_addr, &laddr);
				thistime = realtime();

				if(addressed_to_me)
					break;
			}
		}

		uintptr_t *cur = arr;
		while((cur = (uintptr_t *) *cur) != arr);

		struct ether_hdr *eth = rte_pktmbuf_mtod(packet, struct ether_hdr *);
		eth->d_addr = eth->s_addr;
		eth->s_addr = laddr;

		clock_t delta = realtime() - thistime;
		printf("Computed in: %ld us\n", delta);
		if(iter >= WARMUP) {
#ifdef AVE_MEDIAN
			assert(iter - WARMUP < TIMING_BUFFER_LEN);
			times[iter - WARMUP] = delta;
#else
			ave += delta;
#endif
		}
		++iter;

		if(!rte_eth_tx_burst(PORT, 0, &packet, 1)) {
			fputs("Data wasn't actually transmitted\n", stderr);
			ret = 2;
			goto cleanup;
		}
	}
stoploop:

cleanup:
	munmap(arr, args->len);
	return ret;
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
			printf("USAGE: %s [-l #] [-- <DPDK arguments>...]\n", argv[0]);
			printf(
					" -l #: LENGTH to traverse (octawords, default %d)\n",
					DEFAULT_LEN);
			return 127;
		}

	if(!dpdk_start(argc, argv))
		return 127;

	return experiment(&args);
}

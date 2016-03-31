#include <sys/mman.h>
#include <assert.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_LEN 1
#define MBUF_CACHE_SIZE 17
#define NUM_MBUFS 255
#define PORT 0
#define RX_RING_SIZE 32
#define TX_RING_SIZE 64

static const struct rte_eth_conf PORT_CONF;
static const char *DEFAULT_EAL_ARGS[] = {
	"-m8",
	"--huge-unlink",
};
#define DEFAULT_EAL_NARGS (sizeof DEFAULT_EAL_ARGS / sizeof *DEFAULT_EAL_ARGS)

typedef struct {
	int len;
} args_t;

static bool loop = true;

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

	struct sigaction sigint = {.sa_handler = sigint_handler};
	if(sigaction(SIGINT, &sigint, NULL)) {
		perror("Installing sigint handler");
		ret = 1;
		goto cleanup;
	}

	struct ether_addr laddr;
	rte_eth_macaddr_get(PORT, &laddr);
	printf("Listening on MAC: ");
	paddr(&laddr);

	while(loop) {
		puts("Awaiting client request...");
		struct rte_mbuf *packet = NULL;
		while(true) {
			if(!loop)
				goto stoploop;
			uint16_t ct = rte_eth_rx_burst(PORT, 0, &packet, 1);
			if(ct) {
				struct ether_hdr *eth = rte_pktmbuf_mtod(packet, struct ether_hdr *);
				bool addressed_to_me = false;
#ifndef NDEBUG
				addressed_to_me = is_same_ether_addr(&eth->d_addr, &laddr);
				printf("Received packet%s addressed to me\n", addressed_to_me ? "" : " not");
				printf("\tDestination: ");
				paddr(&eth->d_addr);
				printf("\tSource:      ");
				paddr(&eth->s_addr);
#endif
				if(addressed_to_me)
					break;
			}
		}

#ifndef NDEBUG
		puts("Performing pointer chasing...");
#endif
		uintptr_t *cur = arr;
		while((cur = (uintptr_t *) *cur) != arr);

		struct ether_hdr *eth = rte_pktmbuf_mtod(packet, struct ether_hdr *);
		eth->d_addr = eth->s_addr;
		eth->s_addr = laddr;
		if(!rte_eth_tx_burst(PORT, 0, &packet, 1)) {
			fputs("Data wasn't actually transmitted\n", stderr);
			ret = 2;
			goto cleanup;
		}
		puts("Sent completion notification!");
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

static void perr(const char *s, int no) {
	fprintf(stderr, "%s: %s\n", s, rte_strerror(no));
}

static void perror_rte(const char *s) {
	perr(s, rte_errno);
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

	int leftoverc = 1 + argc - optind + DEFAULT_EAL_NARGS;
	printf("%d\n", leftoverc);
	const char *leftoverv[leftoverc];
	leftoverv[0] = argv[0];
	memcpy(leftoverv + 1, argv + optind, (argc - optind) * sizeof *leftoverv);
	memcpy(leftoverv + argc - optind + 1, DEFAULT_EAL_ARGS, DEFAULT_EAL_NARGS * sizeof *leftoverv);

	if(rte_eal_init(leftoverc, (char **) leftoverv) < 0) {
		perror_rte("Initializing EAL");
		return 127;
	}

	if(!rte_eth_dev_count()) {
		fputs("Error: number of ports must be nonzero\n", stderr);
		return 127;
	}

	if(rte_eth_dev_configure(PORT, 1, 1, &PORT_CONF)) {
		perror_rte("Configuring Ethernet device");
		return 127;
	}

	struct rte_mempool *pool = rte_pktmbuf_pool_create("", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if(!pool) {
		perror_rte("Allocating mbuf pool");
		return 127;
	}

	int errcode;
	if((errcode = rte_eth_rx_queue_setup(PORT, 0, RX_RING_SIZE, rte_eth_dev_socket_id(PORT), NULL, pool))) {
		perr("Setting up RX queue", -errcode);
		return 127;
	}

	if((errcode = rte_eth_tx_queue_setup(PORT, 0, TX_RING_SIZE, rte_eth_dev_socket_id(PORT), NULL))) {
		perr("Setting up TX queue", -errcode);
		return 127;
	}

	if((errcode = rte_eth_dev_start(PORT))) {
		perr("Starting Ethernet device", -errcode);
		return 127;
	}

	return experiment(&args);
}

#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "dpdk_wrapper.h"

#define ITERATIONS 10
#define TIMEOUT_S  5

typedef struct {
	struct ether_addr mac;
} args_t;

static bool timeout;

static void sigalrm_handler(int ign) {
	(void) ign;
	timeout = true;
}

static inline clock_t realtime(void) {
	struct timespec ns;
	clock_gettime(CLOCK_REALTIME, &ns);
	return ns.tv_sec * 1000000 + ns.tv_nsec / 1000;
}

static int experiment(args_t *args, struct rte_mempool *pool) {
	struct sigaction action = {.sa_handler = sigalrm_handler};
	if(sigaction(SIGALRM, &action, NULL)) {
		perror("sigaction()");
		return 1;
	}

	clock_t ave = 0;
	bool subseq = false;
	// Run one extra time and discard the results of the first trial.
	for(int times = 0; times <= ITERATIONS; ++times) {
		struct rte_mbuf *packet = rte_pktmbuf_alloc(pool);
		if(!packet) {
			fputs("Couldn't allocate packet buffer\n", stderr);
			return 2;
		}
		size_t pkt_size = sizeof(struct ether_hdr);
		packet->data_len = pkt_size;
		packet->pkt_len = pkt_size;

		struct ether_hdr *frame = rte_pktmbuf_mtod(packet, struct ether_hdr *);
		rte_eth_macaddr_get(PORT, &frame->s_addr);
		frame->d_addr = args->mac;
		frame->ether_type = 0;

		struct rte_eth_dev_tx_buffer *buf = rte_zmalloc_socket("", RTE_ETH_TX_BUFFER_SIZE(1), 0, rte_eth_dev_socket_id(PORT));
		if(rte_eth_tx_buffer_init(buf, RTE_ETH_TX_BUFFER_SIZE(1))) {
			fputs("Failed to allocate buffer\n", stderr);
			return 3;
		}

		while(true) {
			puts("About to send activation packet!");
			clock_t time = realtime();
			if(!rte_eth_tx_buffer(PORT, 0, buf, packet)) {
				time = realtime();
				if(!rte_eth_tx_buffer_flush(PORT, 0, buf)) {
					fputs("Data was really not actually transmitted\n", stderr);
					return 4;
				}
			}
			puts("Sent activation packet!");

			timeout = false;
			alarm(TIMEOUT_S);
			uint16_t got;
			while(!(got = rte_eth_rx_burst(PORT, 0, &packet, 1)) && !timeout);
			if(got) {
				clock_t duration = realtime() - time;
				if(subseq)
					ave += duration;

				printf("Completed after: %ld us\n", duration);
				subseq = true;
				break;
			}

			puts("Timed out; retrying...");
			// But don't retry after the first trial!
			if(subseq) {
				fprintf("Timed out on %dth iteration\n", time);
				return 5;
			}
		}
	}

	printf("Average: %f us\n", (double) ave / 10);
	return 0;
}

int main(int argc, char **argv) {
	args_t args = {
		.mac = {.addr_bytes = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	};

	int each_arg;
	while((each_arg = getopt(argc, argv, "m:")) != -1)
		switch(each_arg) {
		case 'm':
			if(sscanf(optarg, "%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8, args.mac.addr_bytes, args.mac.addr_bytes + 1,
					args.mac.addr_bytes + 2, args.mac.addr_bytes + 3, args.mac.addr_bytes + 4, args.mac.addr_bytes + 5) != 6) {
				perror("Parsing provided MAC");
				return 127;
			}
			break;
		default:
			printf("USAGE: %s [-m #] [-- <DPDK arguments>...]\n", argv[0]);
			printf(
					" -m #:destination MAC address\n"
					);
			return 127;
		}

	struct rte_mempool *pool = NULL;
	if(!(pool = dpdk_start(argc, argv)))
		return 127;

	return experiment(&args, pool);
}

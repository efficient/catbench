#include <rte_ethdev.h>
#include <unistd.h>

#include "dpdk_wrapper.h"

typedef struct {
	struct ether_addr mac;
} args_t;

static int experiment(args_t *args, struct rte_mempool *pool) {
	int ret = 0;
	struct rte_mbuf *packet = rte_pktmbuf_alloc(pool);
	if(!packet) {
		fputs("Couldn't allocate packet buffer\n", stderr);
		return 1;
	}

	struct ether_hdr *frame = rte_pktmbuf_mtod(packet, struct ether_hdr *);
	rte_pktmbuf_data_len(packet) = sizeof *frame;
	frame->d_addr = args->mac;
	struct ether_addr mine;
	rte_eth_macaddr_get(PORT, &mine);
	frame->s_addr = mine;
	frame->ether_type = 0;

	puts("Sending activation packet!");
	if(!rte_eth_tx_burst(PORT, 0, &packet, 1)) {
		fputs("Data wasn't actually transmitted\n", stderr);
		ret = 2;
		goto cleanup;
	}

	while(!rte_eth_rx_burst(PORT, 0, &packet, 1));
	puts("Received completion confirmation!");

cleanup:
	rte_pktmbuf_free(packet);
	return ret;
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

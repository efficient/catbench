#include <rte_ethdev.h>
#include <rte_malloc.h>
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
		ret = 3;
	}

	sleep(10);
	puts("About to send activation packet!");
	if(!rte_eth_tx_buffer(PORT, 0, buf, packet))
		fputs("Data wasn't actually transmitted\n", stderr);
	if(!rte_eth_tx_buffer_flush(PORT, 0, buf)) {
		fputs("Data was really not actually transmitted\n", stderr);
		ret = 2;
		goto cleanup;
	}
	puts("Sent activation packet!");

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

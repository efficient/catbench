#include "dpdk_wrapper.h"

#include <rte_ethdev.h>
#include <unistd.h>

#define LINK_SPEED ETH_LINK_SPEED_10G
#define MBUF_CACHE_SIZE 250
#define NUM_MBUFS 8191
#define RX_RING_SIZE 128
#define TX_RING_SIZE 512

static const struct rte_eth_conf PORT_CONF = {
	.link_speeds = LINK_SPEED,
};
static const struct rte_eth_rxconf RX_CONF;
static const struct rte_eth_txconf TX_CONF;
static const char *DEFAULT_EAL_ARGS[] = {
	"--socket-mem=64",
	"--huge-unlink",
};
#define DEFAULT_EAL_NARGS (sizeof DEFAULT_EAL_ARGS / sizeof *DEFAULT_EAL_ARGS)

struct rte_mempool *dpdk_start(int argc, char **argv) {
	int leftoverc = 1 + argc - optind + DEFAULT_EAL_NARGS;
	const char *leftoverv[leftoverc];
	leftoverv[0] = argv[0];
	memcpy(leftoverv + 1, argv + optind, (argc - optind) * sizeof *leftoverv);
	memcpy(leftoverv + argc - optind + 1, DEFAULT_EAL_ARGS, DEFAULT_EAL_NARGS * sizeof *leftoverv);

	if(rte_eal_init(leftoverc, (char **) leftoverv) < 0) {
		perror_rte("Initializing EAL");
		return NULL;
	}

	if(!rte_eth_dev_count()) {
		fputs("Error: number of ports must be nonzero\n", stderr);
		return NULL;
	}

	if(rte_eth_dev_configure(PORT, 1, 1, &PORT_CONF)) {
		perror_rte("Configuring Ethernet device");
		return NULL;
	}

	struct rte_mempool *pool = rte_pktmbuf_pool_create("", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if(!pool) {
		perror_rte("Allocating mbuf pool");
		return NULL;
	}

	int errcode;
	if((errcode = rte_eth_rx_queue_setup(PORT, 0, RX_RING_SIZE, rte_eth_dev_socket_id(PORT), &RX_CONF, pool))) {
		perr("Setting up RX queue", -errcode);
		return NULL;
	}

	if((errcode = rte_eth_tx_queue_setup(PORT, 0, TX_RING_SIZE, rte_eth_dev_socket_id(PORT), &TX_CONF))) {
		perr("Setting up TX queue", -errcode);
		return NULL;
	}

	if((errcode = rte_eth_dev_start(PORT))) {
		perr("Starting Ethernet device", -errcode);
		return NULL;
	}

	puts("About to wait for init to complete...");
	struct rte_eth_link status = {.link_status = ETH_LINK_DOWN};
	while(status.link_status != ETH_LINK_UP)
		rte_eth_link_get(PORT, &status);
	printf("Link speed is %u Mbps.\n", status.link_speed);
	if(rte_eth_speed_bitflag(status.link_speed, 0) != LINK_SPEED) {
		fputs("It's the wrong speed, Gromit!\n", stderr);
		return NULL;
	}
	puts("Initialization complete!");

	return pool;
}

#include "dpdk_wrapper.h"

#include <rte_ethdev.h>
#include <unistd.h>

#define MBUF_CACHE_SIZE 17
#define NUM_MBUFS 255
#define RX_RING_SIZE 32
#define TX_RING_SIZE 64

static const struct rte_eth_conf PORT_CONF;
static const char *DEFAULT_EAL_ARGS[] = {
	"-m8",
	"--huge-unlink",
};
#define DEFAULT_EAL_NARGS (sizeof DEFAULT_EAL_ARGS / sizeof *DEFAULT_EAL_ARGS)

bool dpdk_start(int argc, char **argv) {
	int leftoverc = 1 + argc - optind + DEFAULT_EAL_NARGS;
	printf("%d\n", leftoverc);
	const char *leftoverv[leftoverc];
	leftoverv[0] = argv[0];
	memcpy(leftoverv + 1, argv + optind, (argc - optind) * sizeof *leftoverv);
	memcpy(leftoverv + argc - optind + 1, DEFAULT_EAL_ARGS, DEFAULT_EAL_NARGS * sizeof *leftoverv);

	if(rte_eal_init(leftoverc, (char **) leftoverv) < 0) {
		perror_rte("Initializing EAL");
		return false;
	}

	if(!rte_eth_dev_count()) {
		fputs("Error: number of ports must be nonzero\n", stderr);
		return false;
	}

	if(rte_eth_dev_configure(PORT, 1, 1, &PORT_CONF)) {
		perror_rte("Configuring Ethernet device");
		return false;
	}

	struct rte_mempool *pool = rte_pktmbuf_pool_create("", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if(!pool) {
		perror_rte("Allocating mbuf pool");
		return false;
	}

	int errcode;
	if((errcode = rte_eth_rx_queue_setup(PORT, 0, RX_RING_SIZE, rte_eth_dev_socket_id(PORT), NULL, pool))) {
		perr("Setting up RX queue", -errcode);
		return false;
	}

	if((errcode = rte_eth_tx_queue_setup(PORT, 0, TX_RING_SIZE, rte_eth_dev_socket_id(PORT), NULL))) {
		perr("Setting up TX queue", -errcode);
		return false;
	}

	if((errcode = rte_eth_dev_start(PORT))) {
		perr("Starting Ethernet device", -errcode);
		return false;
	}

	return true;
}

#include "cpu_support.h"

#include <stdlib.h>
#include <string.h>

#include "cpuid.h"
#include "log.h"

typedef uint32_t (*const reg_selector_f)(const cpuid_t *);

#define GEN_REG_SELECTOR_F(letter) \
	static uint32_t REG_##letter(const cpuid_t *group) { \
		return group->letter; \
	}

GEN_REG_SELECTOR_F(a)
GEN_REG_SELECTOR_F(b)
GEN_REG_SELECTOR_F(c)
GEN_REG_SELECTOR_F(d)

const int badglobal = 5;

typedef struct {
	uint32_t leaf;
	uint32_t subleaf;
} tree_node_t;

typedef struct {
	reg_selector_f reg;
	unsigned shift;
	uint32_t mask;
} bit_range_t;

typedef struct {
	tree_node_t regs;
	bit_range_t bits;
} cpuid_desc_t;

static const tree_node_t NOD_MODL =      {0x01, 0x00};
static const bit_range_t SEL_MODL_CPUS = {REG_b, 16, 0x000000ff};
static const bit_range_t SEL_MODL_HTT =  {REG_d, 28, 0x00000001};

static const cpuid_desc_t VEC_MAXLEAF =  {{0x00, 0x00}, {REG_a, 00, 0xffffffff}};
static const cpuid_desc_t BIT_CAT_PQE =  {{0x07, 0x00}, {REG_b, 15, 0x00000001}};

static const cpuid_desc_t BIT_CAT_L3 =   {{0x10, 0x00}, {REG_b, 01, 0x00000001}};
static const bit_range_t SEL_CBM_LEN =   {REG_a, 00, 0x0000000f};
static const bit_range_t SEL_SHAREABLE = {REG_b, 00, 0xffffffff};
static const bit_range_t SEL_CLASSES =   {REG_d, 00, 0x0000ffff};

static inline void cpuid_packed(cpuid_t *buf, const tree_node_t *node) {
	cpuid(buf, node->leaf, node->subleaf);
}

static inline uint32_t regs_bitrange(const cpuid_t *bits, const bit_range_t *range) {
	return (range->reg(bits) >> range->shift) & range->mask;
}

static inline uint32_t cpuid_bitrange(const cpuid_desc_t *desc) {
	cpuid_t node;
	cpuid_packed(&node, &desc->regs);
	return regs_bitrange(&node, &desc->bits);
}

static void check_cores(cpu_support_t *buf) {
	cpuid_t chip_version;
	cpuid_packed(&chip_version, &NOD_MODL);

	buf->num_cores = regs_bitrange(&chip_version, &SEL_MODL_CPUS);

	if(regs_bitrange(&chip_version, &SEL_MODL_HTT))
		buf->hthreaded = true;
}

static void check_cat_support(cpu_support_t *buf) {
	if(cpuid_bitrange(&VEC_MAXLEAF) < BIT_CAT_L3.regs.leaf) {
		log_msg(LOG_ERROR, "CPU too old to query for CAT support\n");
		return;
	}

	if(!cpuid_bitrange(&BIT_CAT_PQE)) {
		log_msg(LOG_ERROR, "CPU does not support CAT\n");
		return;
	}

	if(!cpuid_bitrange(&BIT_CAT_L3)) {
		log_msg(LOG_ERROR, "CPU does not support CAT for cache level 3\n");
		return;
	}
	buf->num_cat_levels = 1;
	buf->cat_levels = malloc(sizeof(*buf->cat_levels));

	cpuid_t l3_details;
	cpuid(&l3_details, BIT_CAT_L3.regs.leaf, BIT_CAT_L3.bits.shift);
	buf->cat_levels[0].cache_level = 3;
	buf->cat_levels[0].num_ways = regs_bitrange(&l3_details, &SEL_CBM_LEN) + 1;
	buf->cat_levels[0].shared_ways_mask = regs_bitrange(&l3_details, &SEL_SHAREABLE);
	buf->cat_levels[0].num_classes = regs_bitrange(&l3_details, &SEL_CLASSES) + 1;
	buf->cat_levels[0].supported =
			buf->cat_levels[0].num_ways && buf->cat_levels[0].num_classes;
}

void cpu_support(cpu_support_t *buf) {
	memset(buf, 0, sizeof *buf);
	check_cores(buf);
	// TODO: check_cmt_support() ?
	check_cat_support(buf);
}

void cpu_support_cleanup(cpu_support_t *victim) {
	if (victim->cat_levels) {
		free(victim->cat_levels);
		victim->cat_levels = NULL;
	}
	victim->num_cat_levels = 0;
}

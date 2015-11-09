#ifndef CATBENCH_PROC_MANIP_H_
#define CATBENCH_PROC_MANIP_H_

#include <sched.h>
#include <stdint.h>

#define PROC_MANIP_ERR_PARSE_PID 0x1
#define PROC_MANIP_ERR_READ_MASK 0x2
#define PROC_MANIP_ERR_INSF_PRIV 0x4
#define PROC_MANIP_ERR_UNK_FAULT 0x8

// Returns number of threads updated
int proc_manip_rebalance_system(uint8_t *err, const cpu_set_t *cores);

#endif

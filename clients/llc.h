#ifndef LLC_H_
#define LLC_H_

#include <stdbool.h>

#define LLC_ERROR -1

bool llc_init(void);

int llc_line_size(void);
int llc_num_sets(void);
int llc_assoc_ways(void);

void llc_cleanup(void);

#endif

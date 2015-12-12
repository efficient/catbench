#ifndef MOVNT_H_
#define MOVNT_H_

#include <stdint.h>

void movntq4(uint64_t *dest, uint64_t a, uint64_t b, uint64_t c, uint64_t d);

#endif

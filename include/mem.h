#ifndef _6502_MEM_H_
#define _6502_MEM_H_

#include "lib.h"

#include <assert.h>

#define MAX_MEM (64 * 1024) // 64KiB
static_assert(
        MAX_MEM == UINT16_MAX + 1, "Memory should be addressable by 16-bit addresses");

typedef struct {
    BYTE data[MAX_MEM];
} Mem;

// Cycles: 1
// Returns byte copy at `addr`
BYTE memldb(Mem *mem, uint32_t *cycles, WORD addr);

// Cycles: 2
// Expects `addr` to reference the low byte of word to be loaded
// Returns little-endian word copy from `addr` through `addr + 1`
WORD memldw(Mem *mem, uint32_t *cycles, WORD addr);

// Cycles: 1
// Set byte at `addr` to be `b`
void memstb(Mem *mem, uint32_t *cycles, WORD addr, BYTE b);

// Cycles: 2
// Expects `addr` to reference the low byte of word to be loaded
// Set bytes from `addr` through `addr + 1` to be `w` in little-endian
void memstw(Mem *mem, uint32_t *cycles, WORD addr, WORD w);

#endif // _6502_MEM_H_

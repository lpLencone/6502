#ifndef MOS6502_RAM_H_
#define MOS6502_RAM_H_

#include "lib.h"

#include <assert.h>

#define RAM_SIZE (64 * 1024) // 64KiB
static_assert(
        RAM_SIZE == UINT16_MAX + 1, "Memory should be addressable by 16-bit addresses");

typedef struct {
    BYTE data[RAM_SIZE];
} RAM;

// Cycles: 1
// Returns byte copy at `addr`
BYTE memldb(RAM *mem, WORD addr);

// Cycles: 2 
// Expects `addr` to reference the low byte of word to be loaded
// Returns little-endian word copy from `addr` through `addr + 1`
WORD memldw(RAM *mem, WORD addr);

// Cycles: 1
// Set byte at `addr` to be `b`
void memstb(RAM *mem, WORD addr, BYTE b);

// Cycles: 2
// Expects `addr` to reference the low byte of word to be loaded
// Set bytes from `addr` through `addr + 1` to be `w` in little-endian
void memstw(RAM *mem, WORD addr, WORD w);

#endif // MOS6502_RAM_H_

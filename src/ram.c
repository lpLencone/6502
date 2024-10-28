#include "ram.h"

#include <stdio.h>
#include <stdlib.h>

// Cycles: 1
// Returns byte copy at `addr`
BYTE memldb(RAM *mem, WORD addr)
{
    return mem->data[addr];
}

// Cycles: 2
// Expects `addr` to reference the low byte of word to be loaded
// Returns little-endian word copy from `addr` through `addr + 1`
WORD memldw(RAM *mem, WORD addr)
{
    expect(addr < RAM_SIZE - 1, "Address 0x%x cannot be the low byte of a word", addr);
    WORD w = mem->data[addr] & 0xFF;
    return w |= mem->data[addr + 1] << 8;
}

// Cycles: 1
// Set byte at `addr` to be `b`
void memstb(RAM *mem, WORD addr, BYTE b)
{
    mem->data[addr] = b;
}

// Cycles: 2
// Expects `addr` to reference the low byte of word to be loaded
// Set bytes from `addr` through `addr + 1` to be `w` in little-endian
void memstw(RAM *mem, WORD addr, WORD w)
{
    mem->data[addr] = w & 0xFF;
    mem->data[addr + 1] = (w >> 8);
}

#include "mem.h"

#include <stdio.h>
#include <stdlib.h>

// Cycles: 1
// Returns byte copy at `addr`
BYTE memldb(Mem *mem, uint32_t *cycles, WORD addr)
{
    expect(*cycles > 0, "Not enough cycles to complete instruction.");
    return (*cycles)--, mem->data[addr];
}

// Cycles: 2
// Expects `addr` to reference the low byte of word to be loaded
// Returns little-endian word copy from `addr` through `addr + 1`
WORD memldw(Mem *mem, uint32_t *cycles, WORD addr)
{
    expect(*cycles > 1, "Not enough cycles to complete instruction.");
    expect(addr < MAX_MEM - 1, "Address 0x%x cannot be the low byte of a word", addr);
    (*cycles) -= 2;
    WORD w = mem->data[addr] & 0xFF;
    return w |= mem->data[addr + 1] << 8;
}

// Cycles: 1
// Set byte at `addr` to be `b`
void memstb(Mem *mem, uint32_t *cycles, WORD addr, BYTE b)
{
    expect(*cycles > 0, "Not enough cycles to complete instruction.");
    (*cycles)--;
    mem->data[addr] = b;
}

// Cycles: 2
// Expects `addr` to reference the low byte of word to be loaded
// Set bytes from `addr` through `addr + 1` to be `w` in little-endian
void memstw(Mem *mem, uint32_t *cycles, WORD addr, WORD w)
{
    expect(*cycles > 1, "Not enough cycles to complete instruction.");
    expect(addr < MAX_MEM - 1, "Address 0x%x cannot be the low byte of a word", addr);
    (*cycles) -= 2;
    mem->data[addr] = w & 0xFF;
    mem->data[addr + 1] = (w >> 8);
}

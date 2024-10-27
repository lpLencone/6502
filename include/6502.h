#ifndef _6502_H_
#define _6502_H_

#include "lib.h"
#include "mem.h"

// http://www.6502.org/users/obelisk/6502/reference.html#LDA
#define LDA_IMM 0xA9 //  // 2 bytes // 2 cycles
#define LDA_ZPG 0xA5 //  // 2 bytes // 3 cycles
#define LDA_ZPX 0xB5 //  // 2 bytes // 4 cycles
#define LDA_ABS 0xAD //  // 3 bytes // 4 cycles
#define LDA_ABX 0xBD //  // 3 bytes // (4|5) cycles
#define LDA_ABY 0xB9 //  // 3 bytes // (4|5) cycles
#define LDA_IDX 0xA1 //  // 2 bytes // 6 cycles
#define LDA_IDY 0xB1 //  // 2 bytes // (5|6) cycles
// http://www.6502.org/users/obelisk/6502/reference.html#LDX
#define LDX_IMM 0xA2 //  // 2 bytes // 2 cycles
#define LDX_ZPG 0xA6 //  // 2 bytes // 3 cycles

// http://www.6502.org/users/obelisk/6502/reference.html#JSR
#define JSR 0x20 // Jump to Subroutine // 3 bytes // 6 cycles

typedef struct {
    WORD pc; // Program counter
    BYTE s;  // Stack pointer

    BYTE a; // Accumulator
    BYTE x; // Register index X
    BYTE y; // Register indey Y

    // LSB
    BYTE c : 1; // Carry Flag
    BYTE z : 1; // Zero Flag
    BYTE i : 1; // Interrupt Disable
    BYTE d : 1; // Decimal Mode
    BYTE b : 1; // Break Command
    BYTE _ : 1; // UNUSED
    BYTE o : 1; // Overflow Flag
    BYTE n : 1; // Negative Flag
    // MSB
} _6502;

void _6502_exec(_6502 *cpu, Mem *mem, uint32_t cycles);
void _6502_reset(_6502 *cpu, Mem *mem);

#endif // _6502_H_

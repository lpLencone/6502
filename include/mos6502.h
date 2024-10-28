#ifndef MOS6502_H_
#define MOS6502_H_

#include "lib.h"
#include "ram.h"

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
#define LDX_ZPY 0xB6 //  // 2 bytes // 4 cycles
#define LDX_ABS 0xAE //  // 3 bytes // 4 cycles
#define LDX_ABY 0xBE //  // 3 bytes // (4|5) cycles
// http://www.6502.org/users/obelisk/6502/reference.html#LDY
#define LDY_IMM 0xA0 //  // 2 bytes // 2 cycles
#define LDY_ZPG 0xA4 //  // 2 bytes // 3 cycles
#define LDY_ZPX 0xB4 //  // 2 bytes // 4 cycles
#define LDY_ABS 0xAC //  // 3 bytes // 4 cycles
#define LDY_ABX 0xBC //  // 3 bytes // (4|5) cycles
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
} MOS_6502;


uint64_t mos6502_exec(MOS_6502 *cpu, RAM *mem, uint64_t max_cycles);
void mos6502_reset(MOS_6502 *cpu, RAM *mem);

#endif // MOS6502_H_

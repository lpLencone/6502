#include "6502.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Cycles: 1 (memldb)
static BYTE _6502_fetchb(_6502 *cpu, Mem *mem, uint32_t *cycles)
{
    return memldb(mem, cycles, cpu->pc++);
}

// Cycles: 2 (memldw)
static WORD _6502_fetchw(_6502 *cpu, Mem *mem, uint32_t *cycles)
{
    return (cpu->pc += 2, memldw(mem, cycles, cpu->pc - 2));
}

// // Cycles: 1 (memstb)
// static void _6502_pushb(_6502 *cpu, Mem *mem, uint32_t *cycles, BYTE b)
// {
//     memstb(mem, cycles, cpu->s--, b);
// }

// Cycles: 2 (memstw)
static void _6502_pushw(_6502 *cpu, Mem *mem, uint32_t *cycles, WORD w)
{
    (cpu->s -= 2, memstw(mem, cycles, cpu->s + 2, w));
}

typedef enum {
    LD_IM = 0x0100,  // Load Immediate
    LD_ZP = 0x0101,  // Load (from) Zero Page
    LD_ZPX = 0x0102, // Load (from) Zero Page (+) X
} LDIns;

static uint16_t const insmap[0x100] = {
    [LDA_IM] = LD_IM,   //
    [LDA_ZP] = LD_ZP,   //
    [LDA_ZPX] = LD_ZPX, //
    [LDX_IM] = LD_IM,   //
    [LDX_ZP] = LD_ZP,   //
};

static void
_6502_ld(_6502 *cpu, Mem *mem, uint32_t *cycles, LDIns ld_instruction, BYTE *reg)
{
    switch (ld_instruction) {
        case LD_IM: // http://www.6502.org/users/obelisk/6502/addressing.html#IMM
            *reg = _6502_fetchb(cpu, mem, cycles);
            break;
        case LD_ZP: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPG
            *reg = _6502_fetchb(cpu, mem, cycles);
            *reg = memldb(mem, cycles, *reg);
            break;
        case LD_ZPX: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPX
            *reg = _6502_fetchb(cpu, mem, cycles);
            *reg += cpu->x;
            expect(*cycles > 0, "Not enough cycles to complete instruction.");
            (*cycles)--;
            *reg = memldb(mem, cycles, *reg);
            break;
        default:
            assert(0);
    }
    cpu->z = *reg == 0x0;
    cpu->n = *reg >> 7;
}

void _6502_exec(_6502 *cpu, Mem *mem, uint32_t cycles)
{
    while (cycles > 0) {
        BYTE instruction = _6502_fetchb(cpu, mem, &cycles);
        switch (instruction) {
            case LDA_IM:  // http://www.6502.org/users/obelisk/6502/addressing.html#IMM
            case LDA_ZP:  // http://www.6502.org/users/obelisk/6502/addressing.html#ZPG
            case LDA_ZPX: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPX
                _6502_ld(cpu, mem, &cycles, insmap[instruction], &cpu->a);
                continue;

            case LDX_IM: // http://www.6502.org/users/obelisk/6502/addressing.html#IMM
            case LDX_ZP: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPG
                _6502_ld(cpu, mem, &cycles, insmap[instruction], &cpu->x);
                continue;

            case JSR: { // http://www.6502.org/users/obelisk/6502/addressing.html#ABS
                WORD subroutine_addr = _6502_fetchw(cpu, mem, &cycles);
                _6502_pushw(cpu, mem, &cycles, cpu->pc - 1);
                expect(cycles > 0, "Not enough cycles to complete instruction.");
                cycles--;
                cpu->pc = subroutine_addr;
                continue;
            }
        }
        printf("Instruction not handled: 0x%x\n", instruction);
    }
}

void _6502_reset(_6502 *cpu, Mem *mem)
{
    cpu->pc = 0xFFFC;
    cpu->s = 0xFD;
    cpu->c = cpu->z = cpu->i = cpu->d = cpu->b = cpu->o = cpu->n = 0;
    cpu->a = cpu->x = cpu->y = 0;
    memset(mem->data, 0, MAX_MEM);
}

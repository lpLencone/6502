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
    LD_IMM = 0x0100, // Load Immediate
    LD_ZPG = 0x0101, // Load from Zero Page
    LD_ZPX = 0x0102, // Load from Zero Page + X
    LD_ABS = 0x0103, // Load from absolute address
    LD_ABX = 0x0104, // Load from absolute address + X
    LD_ABY = 0x0105, // Load from absolute address + Y
    LD_IDX = 0x0106, // Load from address in X
    LD_IDY = 0x0107, // Load from address in Y
} LDIns;

static uint16_t const insmap[0x100] = {
    [LDA_IMM] = LD_IMM, //
    [LDA_ZPG] = LD_ZPG, //
    [LDA_ZPX] = LD_ZPX, //
    [LDA_ABS] = LD_ABS, //
    [LDA_ABX] = LD_ABX, //
    [LDA_ABY] = LD_ABY, //
    [LDA_IDX] = LD_IDX, //
    [LDA_IDY] = LD_IDY, //

    [LDX_IMM] = LD_IMM, //
    [LDX_ZPG] = LD_ZPG, //

    [LDY_IMM] = LD_IMM, //
    [LDY_ZPG] = LD_ZPG, //
};

static void _6502_ld(_6502 *cpu, Mem *mem, uint32_t *cycles, LDIns instruction, BYTE *reg)
{
    switch (instruction) {
        case LD_IMM: // http://www.6502.org/users/obelisk/6502/addressing.html#IMM
            *reg = _6502_fetchb(cpu, mem, cycles);
            break;

        case LD_ZPG: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPG
            *reg = _6502_fetchb(cpu, mem, cycles);
            *reg = memldb(mem, cycles, *reg);
            break;

        case LD_ZPX: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPX
            *reg = _6502_fetchb(cpu, mem, cycles);
            expect(*cycles > 0, "Not enough cycles to complete instruction.");
            (*cycles)--;
            *reg += cpu->x;
            *reg = memldb(mem, cycles, *reg);
            break;

        case LD_ABS: { // http://www.6502.org/users/obelisk/6502/addressing.html#ABS
            WORD addr = _6502_fetchw(cpu, mem, cycles);
            *reg = memldb(mem, cycles, addr);
            break;
        }

        case LD_ABX:   // http://www.6502.org/users/obelisk/6502/addressing.html#ABX
        case LD_ABY: { // http://www.6502.org/users/obelisk/6502/addressing.html#ABY
            WORD addr = _6502_fetchw(cpu, mem, cycles);
            BYTE index = (instruction == LD_ABX) ? cpu->x : cpu->y;
            if (((addr + index) & 0xFF) < index) {
                // https://retrocomputing.stackexchange.com/a/146
                expect(*cycles > 0, "Not enough cycles to complete instruction.");
                (*cycles)--;
            }
            addr += index;
            *reg = memldb(mem, cycles, addr);
            break;
        }

        case LD_IDX: { // http://www.6502.org/users/obelisk/6502/addressing.html#IDX
            BYTE addr = _6502_fetchb(cpu, mem, cycles);
            expect(*cycles > 0, "Not enough cycles to complete instruction.");
            (*cycles)--;
            addr += cpu->x;
            addr = memldw(mem, cycles, addr);
            *reg = memldb(mem, cycles, addr);
            break;
        }

        case LD_IDY: { // http://www.6502.org/users/obelisk/6502/addressing.html#IDY
            // TODO: figure out how this could use only 5 cycles (counting reading the
            // instruction)
            WORD addr = _6502_fetchb(cpu, mem, cycles);
            addr = memldw(mem, cycles, addr);
            expect(*cycles > 0, "Not enough cycles to complete instruction.");
            (*cycles)--;
            addr += cpu->y;
            *reg = memldb(mem, cycles, addr);
            break;
        }

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
            case LDA_IMM:
            case LDA_ZPG:
            case LDA_ZPX:
            case LDA_ABS:
            case LDA_ABX:
            case LDA_ABY:
            case LDA_IDX:
            case LDA_IDY:
                _6502_ld(cpu, mem, &cycles, insmap[instruction], &cpu->a);
                continue;

            case LDX_IMM:
            case LDX_ZPG:
                _6502_ld(cpu, mem, &cycles, insmap[instruction], &cpu->x);
                continue;

            case LDY_IMM:
            case LDY_ZPG:
                _6502_ld(cpu, mem, &cycles, insmap[instruction], &cpu->y);
                continue;

            case JSR: {
                WORD subroutine_addr = _6502_fetchw(cpu, mem, &cycles);
                _6502_pushw(cpu, mem, &cycles, cpu->pc - 1);
                expect(cycles > 0, "Not enough cycles to complete instruction.");
                cycles--;
                cpu->pc = subroutine_addr;
                continue;
            }
        }

        panic("Instruction not handled: 0x%x\n", instruction);
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

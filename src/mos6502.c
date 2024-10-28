#include "mos6502.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Cycles: 1 (memldb)
static BYTE mos6502_fetchb(MOS_6502 *cpu, RAM *mem)
{
    return memldb(mem, cpu->pc++);
}

// Cycles: 2 (memldw)
static WORD mos6502_fetchw(MOS_6502 *cpu, RAM *mem)
{
    return (cpu->pc += 2, memldw(mem, cpu->pc - 2));
}

// // Cycles: 1 (memstb)
// static void mos6502_pushb(MOS_6502 *cpu, RAM *mem, uint32_t *cycles, BYTE b)
// {
//     cpu->s --, memstw(mem, cpu->s + 1, w);
// }

// Cycles: 2 (memstw)
static void mos6502_pushw(MOS_6502 *cpu, RAM *mem, WORD w)
{
    cpu->s -= 2, memstw(mem, cpu->s + 2, w);
}

typedef enum {
    LD_IMM = 0x0100, // Load Immediate
    LD_ZPG = 0x0101, // Load from Zero Page
    LD_ZPX = 0x0102, // Load from Zero Page + X
    LD_ZPY = 0x0103, // Load from Zero Page + Y
    LD_ABS = 0x0104, // Load from absolute address
    LD_ABX = 0x0105, // Load from absolute address + X
    LD_ABY = 0x0106, // Load from absolute address + Y
    LD_IDX = 0x0107, // Load from address in X
    LD_IDY = 0x0108, // Load from address in Y
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
    [LDX_ZPY] = LD_ZPY, //
    [LDX_ABS] = LD_ABS, //
    [LDX_ABY] = LD_ABY, //

    [LDY_IMM] = LD_IMM, //
    [LDY_ZPG] = LD_ZPG, //
    [LDY_ZPX] = LD_ZPX, //
    [LDY_ABS] = LD_ABS, //
    [LDY_ABX] = LD_ABX, //
};

static uint64_t mos6502_ld(MOS_6502 *cpu, RAM *mem, LDIns instruction, BYTE *reg)
{
    uint64_t cycles;
    switch (instruction) {
        case LD_IMM: // http://www.6502.org/users/obelisk/6502/addressing.html#IMM
            *reg = mos6502_fetchb(cpu, mem);
            defer(cycles = 2);

        case LD_ZPG: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPG
            *reg = mos6502_fetchb(cpu, mem);
            *reg = memldb(mem, *reg);
            defer(cycles = 3);

        case LD_ZPX: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPX
            expect(reg != &cpu->x, "Cannot apply instruction to register X.");
            *reg = mos6502_fetchb(cpu, mem);
            *reg += cpu->x;
            *reg = memldb(mem, *reg);
            defer(cycles = 4);

        case LD_ZPY: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPY
            expect(reg == &cpu->x, "Cannot apply instruction but to register X");
            *reg = mos6502_fetchb(cpu, mem);
            *reg += cpu->y;
            *reg = memldb(mem, *reg);
            defer(cycles = 4);

        case LD_ABS: { // http://www.6502.org/users/obelisk/6502/addressing.html#ABS
            WORD addr = mos6502_fetchw(cpu, mem);
            *reg = memldb(mem, addr);
            defer(cycles = 4);
        }

        case LD_ABX: { // http://www.6502.org/users/obelisk/6502/addressing.html#ABX
            expect(reg != &cpu->x, "Cannot apply instruction to register X.");
            WORD addr = mos6502_fetchw(cpu, mem);
            addr += cpu->x;
            *reg = memldb(mem, addr);
            // https://retrocomputing.stackexchange.com/a/146
            if ((addr & 0xFF) < cpu->x) {
                defer(cycles = 5);
            }
            defer(cycles = 4);
        }

        case LD_ABY: { // http://www.6502.org/users/obelisk/6502/addressing.html#ABY
            expect(reg != &cpu->y, "Cannot apply instruction to register Y.");
            WORD addr = mos6502_fetchw(cpu, mem);
            addr += cpu->y;
            *reg = memldb(mem, addr);
            // https://retrocomputing.stackexchange.com/a/146
            if ((addr & 0xFF) < cpu->y) {
                defer(cycles = 5);
            }
            defer(cycles = 4);
        }

        case LD_IDX: { // http://www.6502.org/users/obelisk/6502/addressing.html#IDX
            expect(reg == &cpu->a, "Cannot apply instruction but to register A.");
            BYTE addr = mos6502_fetchb(cpu, mem);
            addr += cpu->x;
            addr = memldw(mem, addr);
            *reg = memldb(mem, addr);
            defer(cycles = 6);
        }

        case LD_IDY: { // http://www.6502.org/users/obelisk/6502/addressing.html#IDY
            expect(reg == &cpu->a, "Cannot apply instruction but to register A.");
            WORD addr = mos6502_fetchb(cpu, mem);
            addr = memldw(mem, addr);
            addr += cpu->y;
            *reg = memldb(mem, addr);
            // TODO: figure out exactly the reason
            if ((addr & 0xFF) < cpu->y) {
                defer(cycles = 6);
            }
            defer(cycles = 5);
        }

        default:
            assert(0);
    }

defer:
    cpu->z = *reg == 0x0;
    cpu->n = *reg >> 7;
    return cycles;
}

uint64_t mos6502_exec(MOS_6502 *cpu, RAM *mem, uint64_t max_cycles)
{
    uint64_t cycles = 0;
    while (cycles < max_cycles) {
        BYTE instruction = mos6502_fetchb(cpu, mem);
        switch (instruction) {
            case LDA_IMM:
            case LDA_ZPG:
            case LDA_ZPX:
            case LDA_ABS:
            case LDA_ABX:
            case LDA_ABY:
            case LDA_IDX:
            case LDA_IDY:
                cycles += mos6502_ld(cpu, mem, insmap[instruction], &cpu->a);
                continue;

            case LDX_IMM:
            case LDX_ZPG:
            case LDX_ZPY:
            case LDX_ABS:
            case LDX_ABY:
                cycles += mos6502_ld(cpu, mem, insmap[instruction], &cpu->x);
                continue;

            case LDY_IMM:
            case LDY_ZPG:
            case LDY_ZPX:
            case LDY_ABS:
            case LDY_ABX:
                cycles += mos6502_ld(cpu, mem, insmap[instruction], &cpu->y);
                continue;

            case JSR: {
                WORD subroutine_addr = mos6502_fetchw(cpu, mem);
                mos6502_pushw(cpu, mem, cpu->pc - 1);
                cpu->pc = subroutine_addr;
                cycles += 6;
                continue;
            }
        }

        panic("Instruction not handled: 0x%x\n", instruction);
    }
    return cycles;
}

void mos6502_reset(MOS_6502 *cpu, RAM *mem)
{
    cpu->pc = 0xFFFC;
    cpu->s = 0xFD;
    cpu->c = cpu->z = cpu->i = cpu->d = cpu->b = cpu->o = cpu->n = 0;
    cpu->a = cpu->x = cpu->y = 0;
    memset(mem->data, 0, RAM_SIZE);
}

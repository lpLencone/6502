#include "mos6502.h"
#include "lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BYTE mos6502_fetchb(MOS_6502 *cpu, RAM *mem)
{
    return memldb(mem, cpu->pc++);
}

static WORD mos6502_fetchw(MOS_6502 *cpu, RAM *mem)
{
    return (cpu->pc += 2, memldw(mem, cpu->pc - 2));
}

// static void mos6502_pushb(MOS_6502 *cpu, RAM *mem, uint32_t *cycles, BYTE b)
// {
//     cpu->s --, memstw(mem, cpu->s + 1, w);
// }

static void mos6502_pushw(MOS_6502 *cpu, RAM *mem, WORD w)
{
    cpu->s -= 2, memstw(mem, cpu->s + 2, w);
}

typedef enum {
    AddrMode_IMM, // Immediate
    AddrMode_ZPG, // from Zero Page
    AddrMode_ZPX, // from Zero Page + X
    AddrMode_ZPY, // from Zero Page + Y
    AddrMode_ABS, // from absolute address
    AddrMode_ABX, // from absolute address + X
    AddrMode_ABY, // from absolute address + Y
    AddrMode_IDX, // from address in X
    AddrMode_IDY, // from address in Y
} AddrMode;

char const *modename(AddrMode mode)
{
    switch (mode) {
        case AddrMode_IMM:
            return "AddrMode_IMM";
        case AddrMode_ZPG:
            return "AddrMode_ZPG";
        case AddrMode_ZPX:
            return "AddrMode_ZPX";
        case AddrMode_ZPY:
            return "AddrMode_ZPY";
        case AddrMode_ABS:
            return "AddrMode_ABS";
        case AddrMode_ABX:
            return "AddrMode_ABX";
        case AddrMode_ABY:
            return "AddrMode_ABY";
        case AddrMode_IDX:
            return "AddrMode_IDX";
        case AddrMode_IDY:
            return "AddrMode_IDY";
        default:
            return "Unknown AddrMode";
    }
}

static AddrMode const modemap[0x100] = {
    [LDA_IMM] = AddrMode_IMM, //
    [LDA_ZPG] = AddrMode_ZPG, //
    [LDA_ZPX] = AddrMode_ZPX, //
    [LDA_ABS] = AddrMode_ABS, //
    [LDA_ABX] = AddrMode_ABX, //
    [LDA_ABY] = AddrMode_ABY, //
    [LDA_IDX] = AddrMode_IDX, //
    [LDA_IDY] = AddrMode_IDY, //

    [LDX_IMM] = AddrMode_IMM, //
    [LDX_ZPG] = AddrMode_ZPG, //
    [LDX_ZPY] = AddrMode_ZPY, //
    [LDX_ABS] = AddrMode_ABS, //
    [LDX_ABY] = AddrMode_ABY, //

    [LDY_IMM] = AddrMode_IMM, //
    [LDY_ZPG] = AddrMode_ZPG, //
    [LDY_ZPX] = AddrMode_ZPX, //
    [LDY_ABS] = AddrMode_ABS, //
    [LDY_ABX] = AddrMode_ABX, //

    [STA_ZPG] = AddrMode_ZPG, //
    [STA_ZPX] = AddrMode_ZPX, //
    [STA_ABS] = AddrMode_ABS, //
    [STA_ABX] = AddrMode_ABX, //
    [STA_ABY] = AddrMode_ABY, //
    [STA_IDX] = AddrMode_IDX, //
    [STA_IDY] = AddrMode_IDY, //

    [STX_ZPG] = AddrMode_ZPG, //
    [STX_ZPY] = AddrMode_ZPY, //
    [STX_ABS] = AddrMode_ABS, //

    [STY_ZPG] = AddrMode_ZPG, //
    [STY_ZPX] = AddrMode_ZPX, //
    [STY_ABS] = AddrMode_ABS, //
};

static WORD mos6502_getaddr(MOS_6502 *cpu, RAM *mem, AddrMode mode)
{
    switch (mode) {
        case AddrMode_ZPG: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPG
            return mos6502_fetchb(cpu, mem);

        case AddrMode_ZPX: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPX
            return mos6502_fetchb(cpu, mem) + cpu->x;

        case AddrMode_ZPY: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPY
            return mos6502_fetchb(cpu, mem) + cpu->y;

        case AddrMode_ABS: // http://www.6502.org/users/obelisk/6502/addressing.html#ABS
            return mos6502_fetchw(cpu, mem);

        case AddrMode_ABX: // http://www.6502.org/users/obelisk/6502/addressing.html#ABX
        case AddrMode_ABY: // http://www.6502.org/users/obelisk/6502/addressing.html#ABY
            return mos6502_fetchw(cpu, mem) + ((mode == AddrMode_ABX) ? cpu->x : cpu->y);

        case AddrMode_IDX:
            return memldw(mem, mos6502_fetchb(cpu, mem) + cpu->x);

        case AddrMode_IDY:
            return memldw(mem, mos6502_fetchb(cpu, mem)) + cpu->y;

        default:
            panic("Mode \"%s\" not implemented.", modename(mode));
    }
}

static uint64_t mos6502_ld(MOS_6502 *cpu, RAM *mem, AddrMode mode, BYTE *reg)
{
    uint64_t cycles;
    // http://www.6502.org/users/obelisk/6502/addressing.html#IMM
    if (mode == AddrMode_IMM) {
        *reg = mos6502_fetchb(cpu, mem);
        defer(cycles = 2);
    }

    WORD addr = mos6502_getaddr(cpu, mem, mode);
    switch (mode) {
        case AddrMode_ZPG: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPG
            *reg = memldb(mem, addr);
            defer(cycles = 3);

        case AddrMode_ZPX: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPX
            expect(reg != &cpu->x, "Cannot apply instruction to register X.");
            *reg = memldb(mem, addr);
            defer(cycles = 4);

        case AddrMode_ZPY: // http://www.6502.org/users/obelisk/6502/addressing.html#ZPY
            expect(reg == &cpu->x, "Cannot apply instruction but to register X");
            *reg = memldb(mem, addr);
            defer(cycles = 4);

        case AddrMode_ABS: // http://www.6502.org/users/obelisk/6502/addressing.html#ABS
            *reg = memldb(mem, addr);
            defer(cycles = 4);

        case AddrMode_ABX: // http://www.6502.org/users/obelisk/6502/addressing.html#ABX
            expect(reg != &cpu->x, "Cannot apply instruction to register X.");
            *reg = memldb(mem, addr);
            // https://retrocomputing.stackexchange.com/a/146
            if ((addr & 0xFF) < cpu->x) {
                defer(cycles = 5);
            }
            defer(cycles = 4);

        case AddrMode_ABY: // http://www.6502.org/users/obelisk/6502/addressing.html#ABY
            expect(reg != &cpu->y, "Cannot apply instruction to register Y.");
            *reg = memldb(mem, addr);
            // https://retrocomputing.stackexchange.com/a/146
            if ((addr & 0xFF) < cpu->y) {
                defer(cycles = 5);
            }
            defer(cycles = 4);

        case AddrMode_IDX: // http://www.6502.org/users/obelisk/6502/addressing.html#IDX
            expect(reg == &cpu->a, "Cannot apply instruction but to register A.");
            *reg = memldb(mem, addr);
            defer(cycles = 6);

        case AddrMode_IDY: // http://www.6502.org/users/obelisk/6502/addressing.html#IDY
            expect(reg == &cpu->a, "Cannot apply instruction but to register A.");
            *reg = memldb(mem, addr);
            // TODO: figure out exactly the reason
            if ((addr & 0xFF) < cpu->y) {
                defer(cycles = 6);
            }
            defer(cycles = 5);

        default:
            panic("Mode \"%s\" not implemented for Load instructions (LD*)",
                  modename(mode));
    }

defer:
    cpu->z = *reg == 0x0;
    cpu->n = *reg >> 7;
    return cycles;
}

static uint64_t mos6502_st(MOS_6502 *cpu, RAM *mem, AddrMode mode, BYTE *reg)
{
    WORD addr = mos6502_getaddr(cpu, mem, mode);
    switch (mode) {
        case AddrMode_ZPG: {
            memstb(mem, addr, *reg);
            return 3;
        }

        case AddrMode_ZPX: {
            expect(reg != &cpu->x, "Cannot apply instruction to Register X");
            memstb(mem, addr, *reg);
            return 4;
        }

        case AddrMode_ZPY: {
            expect(reg == &cpu->x, "Cannot apply instruction but to Register X");
            memstb(mem, addr, *reg);
            return 4;
        }

        case AddrMode_ABS: {
            memstb(mem, addr, *reg);
            return 4;
        }

        case AddrMode_ABX:
        case AddrMode_ABY: {
            expect(reg == &cpu->a, "Cannot apply instruction but to Accumulator.");
            memstb(mem, addr, *reg);
            return 5;
        }

        case AddrMode_IDX:
        case AddrMode_IDY:
            expect(reg == &cpu->a, "Cannot apply instruction but to Accumulator.");
            memstb(mem, addr, *reg);
            return 6;

        default:
            panic("Mode \"%s\" not implemented for Store instructions (ST*)",
                  modename(mode));
    }
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
                cycles += mos6502_ld(cpu, mem, modemap[instruction], &cpu->a);
                continue;
            case LDX_IMM:
            case LDX_ZPG:
            case LDX_ZPY:
            case LDX_ABS:
            case LDX_ABY:
                cycles += mos6502_ld(cpu, mem, modemap[instruction], &cpu->x);
                continue;
            case LDY_IMM:
            case LDY_ZPG:
            case LDY_ZPX:
            case LDY_ABS:
            case LDY_ABX:
                cycles += mos6502_ld(cpu, mem, modemap[instruction], &cpu->y);
                continue;

            case STA_ZPG:
            case STA_ZPX:
            case STA_ABS:
            case STA_ABX:
            case STA_ABY:
            case STA_IDX:
            case STA_IDY:
                cycles += mos6502_st(cpu, mem, modemap[instruction], &cpu->a);
                continue;
            case STX_ZPG:
            case STX_ZPY:
            case STX_ABS:
                cycles += mos6502_st(cpu, mem, modemap[instruction], &cpu->x);
                continue;
            case STY_ZPG:
            case STY_ZPX:
            case STY_ABS:
                cycles += mos6502_st(cpu, mem, modemap[instruction], &cpu->y);
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

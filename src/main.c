#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib.h"
#include "mos6502.h"

// http://www.6502.org/users/obelisk/6502/index.html
// https://www.c64-wiki.com/wiki/Reset_(Process)

#define ASSERT_EQ(l, r)   expect((l) == (r), "")
#define ASSERT_SET(bit)   expect(bit == 0b1, "");
#define ASSERT_UNSET(bit) expect(bit == 0b0, "");

int main(void)
{
    // Testing memory
    {
        printf("Testing memory...\n");
        RAM mem = { 0 };
        memstw(&mem, 0xFFFE, 0x1234);
        ASSERT_EQ(mem.data[0xFFFE], 0x34);
        ASSERT_EQ(mem.data[0xFFFF], 0x12);
        memstb(&mem, 0xFFFD, 0xAB);
        ASSERT_EQ(mem.data[0xFFFD], 0xAB);
        WORD w = memldw(&mem, 0xFFFD);
        ASSERT_EQ(w, 0x34AB);
        BYTE b = memldb(&mem, 0xFFFF);
        ASSERT_EQ(b, 0x12);
    }

    RAM mem;
    MOS_6502 cpu;

    // Testing 6502 Reset
    {
        printf("Testing 6502 reset...\n");
        mos6502_reset(&cpu, &mem);
        ASSERT_EQ(cpu.pc, 0xFFFC);
        ASSERT_EQ(cpu.s, 0xFD);
    }

    char const *const regnames[] = { "Accumulator", "Register X ",
                                     "Register Y " };
    BYTE *const reg[3] = { &cpu.a, &cpu.x, &cpu.y };

    // Testing LD_IMM Immediate and flags
    {
        uint16_t const ins[3] = { LDA_IMM, LDX_IMM, LDY_IMM };
        for (size_t i = 0; i < 3; i++) {
            printf("Testing Load %s Immediate and flags...\n", regnames[i]);
            mos6502_reset(&cpu, &mem);

            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x0;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 2), 2);
            ASSERT_EQ(*reg[i], 0x0);
            ASSERT_UNSET(cpu.n);
            ASSERT_SET(cpu.z);

            mem.data[cpu.pc] = ins[i];
            mem.data[(WORD) (cpu.pc + 1)] = 0x7F;
            mos6502_exec(&cpu, &mem, 2);
            ASSERT_EQ(*reg[i], 0x7F);
            ASSERT_UNSET(cpu.n);
            ASSERT_UNSET(cpu.z);

            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x80;
            mos6502_exec(&cpu, &mem, 2);
            ASSERT_EQ(*reg[i], 0x80);
            ASSERT_SET(cpu.n);
            ASSERT_UNSET(cpu.z);
        }
    }

    // Testing LD_ZPG
    {
        uint16_t const ins[3] = { LDA_ZPG, LDX_ZPG, LDY_ZPG };
        for (size_t i = 0; i < 3; i++) {
            mos6502_reset(&cpu, &mem);
            printf("Testing Load %s from Zero Page...\n", regnames[i]);
            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x23;
            mem.data[0x23] = 0x42;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 3), 3);
            ASSERT_EQ(*reg[i], 0x42);
        }
    }

    // Testing LD_ZPX
    {
        uint16_t const ins[3] = { LDA_ZPX, 0, LDY_ZPX };
        for (size_t i = 0; i < 3; i += 2) {
            printf("Testing Load %s from Zero Page X...\n", regnames[i]);
            mos6502_reset(&cpu, &mem);
            cpu.x = 0x10;
            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x23;
            mem.data[0x33] = 0xF1;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 4), 4);
            ASSERT_EQ(*reg[i], 0xF1);
        }
    }

    // Testing LD_ZPX
    {
        printf("Testing Load from Zero Page Y...\n");
        mos6502_reset(&cpu, &mem);
        cpu.y = 0x10;
        mem.data[cpu.pc] = LDX_ZPY;
        mem.data[cpu.pc + 1] = 0x23;
        mem.data[0x33] = 0xF1;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 4), 4);
        ASSERT_EQ(cpu.x, 0xF1);
    }

    // Testing LD_ABS
    {
        uint16_t const ins[3] = { LDA_ABS, LDX_ABS, LDY_ABS };
        for (size_t i = 0; i < 3; i++) {
            printf("Testing Load %s From Absolute Address...\n", regnames[i]);
            mos6502_reset(&cpu, &mem);
            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0xFE;
            mem.data[cpu.pc + 2] = 0xCA;
            mem.data[0xCAFE] = 0xF0;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 4), 4);
            ASSERT_EQ(*reg[i], 0xF0);
        }
    }

    // Testing LD_ABX (page not crossed)
    {
        uint16_t const ins[3] = { LDA_ABX, 0, LDY_ABX };
        for (size_t i = 0; i < 3; i += 2) {
            printf("Testing Load %s From Absolute Address X (Page not crossed)...\n",
                   regnames[i]);
            mos6502_reset(&cpu, &mem);
            cpu.x = 0x10;
            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x00;
            mem.data[cpu.pc + 2] = 0x20;
            mem.data[0x2010] = 0xF0;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 4), 4);
            ASSERT_EQ(*reg[i], 0xF0);
        }
    }

    // Testing LD_ABX (page crossed)
    {
        uint16_t const ins[3] = { LDA_ABX, 0, LDY_ABX };
        for (size_t i = 0; i < 3; i += 2) {
            printf("Testing Load %s From Absolute Address X (Page crossed)...\n",
                   regnames[i]);
            mos6502_reset(&cpu, &mem);
            cpu.x = 0xFF;
            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x50;
            mem.data[cpu.pc + 2] = 0x20;
            mem.data[0x214F] = 0x7C;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 5), 5);
            ASSERT_EQ(*reg[i], 0x7C);
        }
    }

    // Testing LD_ABY (page not crossed)
    {
        uint16_t const ins[2] = { LDA_ABY, LDX_ABY };
        for (size_t i = 0; i < 2; i++) {
            printf("Testing Load %s From Absolute Address Y (Page not crossed)...\n",
                   regnames[i]);
            mos6502_reset(&cpu, &mem);
            cpu.y = 0x10;
            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x00;
            mem.data[cpu.pc + 2] = 0x20;
            mem.data[0x2010] = 0xF0;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 4), 4);
            ASSERT_EQ(*reg[i], 0xF0);
        }
    }

    // Testing LD_ABY (page crossed)
    {
        uint16_t const ins[2] = { LDA_ABY, LDX_ABY };
        for (size_t i = 0; i < 2; i++) {
            printf("Testing Load %s From Absolute Address Y (Page crossed)...\n",
                   regnames[i]);
            mos6502_reset(&cpu, &mem);
            cpu.y = 0xFF;
            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x50;
            mem.data[cpu.pc + 2] = 0x20;
            mem.data[0x214F] = 0x7C;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 5), 5);
            ASSERT_EQ(*reg[i], 0x7C);
        }
    }

    // Testing LD_IDX
    {
        printf("Testing Load Indexed Indirect (X)...\n");
        mos6502_reset(&cpu, &mem);
        cpu.x = 0x10;
        mem.data[0x23] = 0xBE;
        mem.data[0x24] = 0xBA;
        mem.data[cpu.pc] = LDA_IDX;
        mem.data[cpu.pc + 1] = 0x13;
        mem.data[0xBABE] = 0x0;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 6), 6);
        ASSERT_EQ(cpu.a, 0x0);
    }

    // Testing LD_IDY (Page not crossed)
    {
        printf("Testing Load Indirect Indexed (Y, Page not crossed)...\n");
        mos6502_reset(&cpu, &mem);
        cpu.y = 0x10;
        mem.data[0x23] = 0xBE;
        mem.data[0x24] = 0xBA;
        mem.data[cpu.pc] = LDA_IDY;
        mem.data[cpu.pc + 1] = 0x23;
        mem.data[0xBACE] = 0x80;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 5), 5);
        ASSERT_EQ(cpu.a, 0x80);
    }

    // Testing LD_IDY (Page crossed)
    {
        printf("Testing Load Indirect Indexed (Y, Page crossed)...\n");
        mos6502_reset(&cpu, &mem);
        cpu.y = 0x50;
        mem.data[0x23] = 0xBE;
        mem.data[0x24] = 0xBA;
        mem.data[cpu.pc] = LDA_IDY;
        mem.data[cpu.pc + 1] = 0x23;
        mem.data[0xBB0E] = 0x80;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 6), 6);
        ASSERT_EQ(cpu.a, 0x80);
    }

    // Testing JSR
    {
        mos6502_reset(&cpu, &mem);
        WORD pc = cpu.pc;
        mem.data[pc++] = JSR;
        mem.data[pc++] = 0x10;
        mem.data[pc++] = 0xFF;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 6), 6);
        ASSERT_EQ(cpu.s, 0xFB);
        ASSERT_EQ(cpu.pc, 0xFF10);
    }

    printf("All tests passed.\n");
}

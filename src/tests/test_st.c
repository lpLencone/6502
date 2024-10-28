#ifndef TEST_ST_C_
#define TEST_ST_C_

#include "lib.h"
#include "mos6502.h"

#include <stdio.h>
#include <stdlib.h>

void test_st(void)
{
    MOS_6502 cpu;
    RAM mem;

    char const *const regnames[] = { "Accumulator", "Register X ", "Register Y " };
    BYTE *const reg[3] = { &cpu.a, &cpu.x, &cpu.y };

    // Testing ST*_ZPG
    {
        uint16_t const ins[3] = { STA_ZPG, STX_ZPG, STY_ZPG };
        for (size_t i = 0; i < 3; i++) {
            printf("Testing Store %s On Zero Page...\n", regnames[i]);
            mos6502_reset(&cpu, &mem);
            *reg[i] = 0x42;
            mem.data[cpu.pc + 0] = ins[i];
            mem.data[cpu.pc + 1] = 0x80;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 3), 3);
            ASSERT_EQ(mem.data[0x80], 0x42);
        }
    }

    // Testing ST*_ZPX
    {
        uint16_t const ins[3] = { STA_ZPX, 0, STY_ZPX };
        for (size_t i = 0; i < 3; i += 2) {
            printf("Testing Store %s On Zero Page X...\n", regnames[i]);
            mos6502_reset(&cpu, &mem);
            *reg[i] = 0x24;
            cpu.x = 0x1E;
            mem.data[cpu.pc + 0] = ins[i];
            mem.data[cpu.pc + 1] = 0x80;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 4), 4);
            ASSERT_EQ(mem.data[0x9E], 0x24);
        }
    }

    // Testing STX_ZPY
    {
        printf("Testing Store %s On Zero Page Y...\n", regnames[1]);
        mos6502_reset(&cpu, &mem);
        cpu.x = 0x3F;
        cpu.y = 0xA0;
        mem.data[cpu.pc + 0] = STX_ZPY;
        mem.data[cpu.pc + 1] = 0x18;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 4), 4);
        ASSERT_EQ(mem.data[0xB8], 0x3F);
    }

    // Testing ST*_ABS
    {
        uint16_t const ins[3] = { STA_ABS, STX_ABS, STY_ABS };
        for (size_t i = 0; i < 3; i++) {
            printf("Testing Store %s On Absolute Address...\n", regnames[i]);
            mos6502_reset(&cpu, &mem);
            *reg[i] = 0x30 + i;
            mem.data[cpu.pc + 0] = ins[i];
            mem.data[cpu.pc + 1] = 0xBE + i;
            mem.data[cpu.pc + 2] = 0xBA;
            ASSERT_EQ(mos6502_exec(&cpu, &mem, 4), 4);
            ASSERT_EQ(mem.data[0xBABE + i], 0x30 + i);
        }
    }

    // Testing STA_ABX
    {
        printf("Testing Store %s On Absolute Address X...\n", regnames[0]);
        mos6502_reset(&cpu, &mem);
        cpu.a = 0x42;
        cpu.x = 0xAA;
        mem.data[cpu.pc + 0] = STA_ABX;
        mem.data[cpu.pc + 1] = 0x10;
        mem.data[cpu.pc + 2] = 0xCC;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 5), 5);
        ASSERT_EQ(mem.data[0xCCBA], 0x42);
    }

    // Testing STA_ABY
    {
        printf("Testing Store %s On Absolute Address Y...\n", regnames[0]);
        mos6502_reset(&cpu, &mem);
        cpu.a = 0x42;
        cpu.y = 0xAA;
        mem.data[cpu.pc + 0] = STA_ABY;
        mem.data[cpu.pc + 1] = 0x10;
        mem.data[cpu.pc + 2] = 0xCC;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 5), 5);
        ASSERT_EQ(mem.data[0xCCBA], 0x42);
    }

    // Testing STA_IDX
    {
        printf("Testing Store %s On Indexed Indirect Address (X)...\n", regnames[0]);
        mos6502_reset(&cpu, &mem);
        cpu.a = 0x42;
        cpu.x = 0x10;
        mem.data[0xCB] = 0x1A;
        mem.data[0xCC] = 0x2A;
        mem.data[cpu.pc + 0] = STA_IDX;
        mem.data[cpu.pc + 1] = 0xBB;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 6), 6);
        ASSERT_EQ(mem.data[0x2A1A], 0x42);
    }

    // Testing STA_IDY
    {
        printf("Testing Store %s On Indirect Indexed Address (Y)...\n", regnames[0]);
        mos6502_reset(&cpu, &mem);
        cpu.a = 0x42;
        cpu.y = 0x10;
        mem.data[0xBB] = 0x1A;
        mem.data[0xBC] = 0x2A;
        mem.data[cpu.pc + 0] = STA_IDY;
        mem.data[cpu.pc + 1] = 0xBB;
        ASSERT_EQ(mos6502_exec(&cpu, &mem, 6), 6);
        ASSERT_EQ(mem.data[0x2A2A], 0x42);
    }
}

#endif // TEST_ST_C_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "6502.h"
#include "lib.h"

#define ASSERT_EQ(l, r)   expect((l) == (r), "")
#define ASSERT_SET(bit)   expect(bit == 0b1, "");
#define ASSERT_UNSET(bit) expect(bit == 0b0, "");

int main(void)
{
    // Testing memory
    {
        printf("Testing memory...\n");
        Mem mem = { 0 };
        uint32_t cycles = 6;
        memstw(&mem, &cycles, 0xFFFE, 0x1234);
        ASSERT_EQ(cycles, 4);
        ASSERT_EQ(mem.data[0xFFFE], 0x34);
        ASSERT_EQ(mem.data[0xFFFF], 0x12);
        memstb(&mem, &cycles, 0xFFFD, 0xAB);
        ASSERT_EQ(cycles, 3);
        ASSERT_EQ(mem.data[0xFFFD], 0xAB);
        WORD w = memldw(&mem, &cycles, 0xFFFD);
        ASSERT_EQ(cycles, 1);
        ASSERT_EQ(w, 0x34AB);
        BYTE b = memldb(&mem, &cycles, 0xFFFF);
        ASSERT_EQ(cycles, 0);
        ASSERT_EQ(b, 0x12);
    }

    // Testing 6502 Reset
    {
        printf("Testing 6502 reset...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);
        ASSERT_EQ(cpu.pc, 0xFFFC);
        ASSERT_EQ(cpu.s, 0xFD);
    }

    // Testing LD_IMM Immediate and flags
    {
        Mem mem;
        _6502 cpu;

        uint16_t const ins[3] = { LDA_IMM, LDX_IMM, LDY_IMM };
        char const *const msg[] = { "Accumulator", "Index Register X", "Index Register Y" };
        BYTE *const reg[3] = {&cpu.a, &cpu.x, &cpu.y};

        for (size_t i = 0; i < 3; i++) {
            printf("Testing Load %s Immediate and flags...\n", msg[i]);
            _6502_reset(&cpu, &mem);

            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x0;
            _6502_exec(&cpu, &mem, 2);
            ASSERT_EQ(*reg[i], 0x0);
            ASSERT_UNSET(cpu.n);
            ASSERT_SET(cpu.z);

            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x7F;
            _6502_exec(&cpu, &mem, 2);
            ASSERT_EQ(*reg[i], 0x7F);
            ASSERT_UNSET(cpu.n);
            ASSERT_UNSET(cpu.z);

            mem.data[cpu.pc] = ins[i];
            mem.data[cpu.pc + 1] = 0x80;
            _6502_exec(&cpu, &mem, 2);
            ASSERT_EQ(*reg[i], 0x80);
            ASSERT_SET(cpu.n);
            ASSERT_UNSET(cpu.z);
        }
    }

    // Testing LD_ZPG
    {
        printf("Testing Load from Zero Page...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);

        mem.data[cpu.pc] = LDA_ZPG;
        mem.data[cpu.pc + 1] = 0x23;
        mem.data[0x23] = 0x42;
        _6502_exec(&cpu, &mem, 3);
        ASSERT_EQ(cpu.a, 0x42);
    }

    // Testing LD_ZPX
    {
        printf("Testing Load from Zero Page X...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);

        cpu.x = 0x10;

        mem.data[cpu.pc] = LDA_ZPX;
        mem.data[cpu.pc + 1] = 0x23;
        mem.data[0x33] = 0xF1;
        _6502_exec(&cpu, &mem, 4);
        ASSERT_EQ(cpu.a, 0xF1);
    }

    // Testing LD_ABS
    {
        printf("Testing Load From Absolute Address...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);

        mem.data[cpu.pc] = LDA_ABS;
        mem.data[cpu.pc + 1] = 0xFE;
        mem.data[cpu.pc + 2] = 0xCA;
        mem.data[0xCAFE] = 0xF0;
        _6502_exec(&cpu, &mem, 4);
        ASSERT_EQ(cpu.a, 0xF0);
    }

    // Testing LD_ABX (page not crossed)
    {
        printf("Testing Load From Absolute Address X (Page not crossed)...\n");
        printf("Testing Load From Absolute Address X (Page not crossed)...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);
        cpu.x = 0x10;

        mem.data[cpu.pc] = LDA_ABX;
        mem.data[cpu.pc + 1] = 0x00;
        mem.data[cpu.pc + 2] = 0x20;
        mem.data[0x2010] = 0xF0;
        _6502_exec(&cpu, &mem, 4);
        ASSERT_EQ(cpu.a, 0xF0);
    }

    // Testing LD_ABX (page crossed)
    {
        printf("Testing Load From Absolute Address X (Page crossed)...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);
        cpu.x = 0xFF;

        mem.data[cpu.pc] = LDA_ABX;
        mem.data[cpu.pc + 1] = 0x50;
        mem.data[cpu.pc + 2] = 0x20;
        mem.data[0x214F] = 0x7C;
        _6502_exec(&cpu, &mem, 5);
        ASSERT_EQ(cpu.a, 0x7C);
    }

    // Testing LD_ABY (page not crossed)
    {
        printf("Testing Load From Absolute Address Y (Page not crossed)...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);
        cpu.y = 0x10;

        mem.data[cpu.pc] = LDA_ABY;
        mem.data[cpu.pc + 1] = 0x00;
        mem.data[cpu.pc + 2] = 0x20;
        mem.data[0x2010] = 0xF0;
        _6502_exec(&cpu, &mem, 4);
        ASSERT_EQ(cpu.a, 0xF0);
    }

    // Testing LD_ABY (page crossed)
    {
        printf("Testing Load From Absolute Address Y (Page crossed)...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);
        cpu.y = 0xFF;

        mem.data[cpu.pc] = LDA_ABY;
        mem.data[cpu.pc + 1] = 0x50;
        mem.data[cpu.pc + 2] = 0x20;
        mem.data[0x214F] = 0x7C;
        _6502_exec(&cpu, &mem, 5);
        ASSERT_EQ(cpu.a, 0x7C);
    }

    // Testing LD_IDX
    {
        printf("Testing Load Indexed Indirect (X)...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);

        cpu.x = 0x10;
        mem.data[0x23] = 0xBE;
        mem.data[0x24] = 0xBA;

        mem.data[cpu.pc] = LDA_IDX;
        mem.data[cpu.pc + 1] = 0x13;
        mem.data[0xBABE] = 0x0;
        _6502_exec(&cpu, &mem, 6);
        ASSERT_EQ(cpu.a, 0x0);
    }

    // Testing LD_IDY
    {
        printf("Testing Load Indirect Indexed (Y)...\n");
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);

        cpu.y = 0x10;
        mem.data[0x23] = 0xBE;
        mem.data[0x24] = 0xBA;

        mem.data[cpu.pc] = LDA_IDY;
        mem.data[cpu.pc + 1] = 0x23;
        mem.data[0xBACE] = 0x80;
        _6502_exec(&cpu, &mem, 6);
        ASSERT_EQ(cpu.a, 0x80);
    }

    // Testing 6502
    {
        Mem mem;
        _6502 cpu;
        _6502_reset(&cpu, &mem);

        WORD pc = cpu.pc;

        mem.data[pc++] = JSR;
        mem.data[pc++] = 0x10;
        mem.data[pc++] = 0xFF;

        mem.data[0xFF10] = LDX_IMM;
        mem.data[0xFF11] = 0x10;

        mem.data[0xFF12] = JSR;
        mem.data[0xFF13] = pc & 0xFF;
        mem.data[0xFF14] = pc >> 8;

        mem.data[pc++] = LDA_ZPX;
        mem.data[pc++] = 0x32;
        mem.data[0x42] = 0xFF;

        // Testing JSR
        {
            _6502_exec(&cpu, &mem, 6);
            ASSERT_EQ(cpu.s, 0xFB);
            ASSERT_EQ(cpu.pc, 0xFF10); // Shouldn't pc decrement?
        }

        // Testing JDX
        {
            _6502_exec(&cpu, &mem, 2);
            ASSERT_EQ(cpu.x, 0x10);
            ASSERT_EQ(cpu.pc, 0xFF12);
        }

        // JSR
        _6502_exec(&cpu, &mem, 6);
        ASSERT_EQ(cpu.s, 0xF9);
        ASSERT_EQ(cpu.pc, 0xFFFF);

        // JDA
        _6502_exec(&cpu, &mem, 4);
        ASSERT_EQ(cpu.a, 0xFF);
        ASSERT_SET(cpu.n); // Negative flag must be set, since bit 7 of `A` is 1
        ASSERT_EQ(cpu.pc, 0x0001);

        assert(cpu.a == 0xFF);
        assert(cpu.x == 0x10);
    }

    printf("All tests passed.\n");
}

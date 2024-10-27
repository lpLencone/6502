#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "6502.h"
#include "lib.h"

#define ASSERT_EQ(l, r) expect((l) == (r), "")
#define ASSERT_SET(bit) expect(bit == 0b1, "");

int main(void)
{
    // Testing memory
    {
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

    // Testing 6502
    {
        Mem mem;
        _6502 cpu;

        // Testing reset
        {
            _6502_reset(&cpu, &mem);
            ASSERT_EQ(cpu.pc, 0xFFFC);
            ASSERT_EQ(cpu.s, 0xFD);
        }

        WORD pc = cpu.pc;

        mem.data[pc++] = JSR;
        mem.data[pc++] = 0x10;
        mem.data[pc++] = 0xFF;

        mem.data[0xFF10] = LDX_IM;
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
        ASSERT_EQ(cpu.pc, 0xFFFF); // Shouldn't pc decrement?

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

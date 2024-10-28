#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib.h"
#include "mos6502.h"

#include "tests/test_ld.c"
#include "tests/test_st.c"

// http://www.6502.org/users/obelisk/6502/index.html
// https://www.c64-wiki.com/wiki/Reset_(Process)

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

    // Testing Load
    test_ld();
    test_st();

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

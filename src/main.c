#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define eprintf(fmt, ...) fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__)
#define panic(fmt, ...)                                      \
    do {                                                     \
        eprintf("%s::%s::%d::" fmt "\n", __FILE__, __func__, \
                __LINE__ __VA_OPT__(, ) __VA_ARGS__);        \
        exit(1);                                             \
    } while (0)
#define expect(cond, fmt, ...)                                                 \
    do {                                                                       \
        if (!(cond)) {                                                         \
            panic("Assersion \"" #cond "\" not satisfied: " fmt __VA_OPT__(, ) \
                          __VA_ARGS__);                                        \
        }                                                                      \
    } while (0)

// http://www.6502.org/users/obelisk/6502/index.html
// https://www.c64-wiki.com/wiki/Reset_(Process)

typedef uint8_t BYTE;
typedef uint16_t WORD;

#define MAX_MEM (64 * 1024) // 64KiB
static_assert(
        MAX_MEM == UINT16_MAX + 1, "Memory should be addressable by 16-bit addresses");

typedef struct {
    BYTE data[MAX_MEM];
} Mem;

// Cycles: 1
// Returns byte copy at `addr`
BYTE memldb(Mem *mem, uint32_t *cycles, WORD addr)
{
    expect(*cycles > 0, "Not enough cycles to complete instruction.");
    return (*cycles)--, mem->data[addr];
}

// Cycles: 2
// Expects `addr` to reference the low byte of word to be loaded
// Returns little-endian word copy from `addr` through `addr + 1`
WORD memldw(Mem *mem, uint32_t *cycles, WORD addr)
{
    expect(*cycles > 1, "Not enough cycles to complete instruction.");
    expect(addr < MAX_MEM - 1, "Address 0x%x cannot be the low byte of a word", addr);
    (*cycles) -= 2;
    WORD w = mem->data[addr] & 0xFF;
    return w |= mem->data[addr + 1] << 8;
}

// Cycles: 1
// Set byte at `addr` to be `b`
void memstb(Mem *mem, uint32_t *cycles, WORD addr, BYTE b)
{
    expect(*cycles > 0, "Not enough cycles to complete instruction.");
    (*cycles)--;
    mem->data[addr] = b;
}

// Cycles: 2
// Expects `addr` to reference the low byte of word to be loaded
// Set bytes from `addr` through `addr + 1` to be `w` in little-endian
void memstw(Mem *mem, uint32_t *cycles, WORD addr, WORD w)
{
    expect(*cycles > 1, "Not enough cycles to complete instruction.");
    expect(addr < MAX_MEM - 1, "Address 0x%x cannot be the low byte of a word", addr);
    (*cycles) -= 2;
    mem->data[addr] = w & 0xFF;
    mem->data[addr + 1] = (w >> 8);
}

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
} _6502;

void _6502_reset(_6502 *cpu, Mem *mem)
{
    cpu->pc = 0xFFFC;
    cpu->s = 0xFD;
    cpu->c = cpu->z = cpu->i = cpu->d = cpu->b = cpu->o = cpu->n = 0;
    cpu->a = cpu->x = cpu->y = 0;
    memset(mem->data, 0, MAX_MEM);
}

// Cycles: 1 (memldb)
BYTE _6502_fetchb(_6502 *cpu, Mem *mem, uint32_t *cycles)
{
    return memldb(mem, cycles, cpu->pc++);
}

// Cycles: 2 (memldw)
WORD _6502_fetchw(_6502 *cpu, Mem *mem, uint32_t *cycles)
{
    return (cpu->pc += 2, memldw(mem, cycles, cpu->pc - 2));
}

// Cycles: 1 (memstb)
void _6502_pushb(_6502 *cpu, Mem *mem, uint32_t *cycles, BYTE b)
{
    memstb(mem, cycles, cpu->s--, b);
}

// Cycles: 2 (memstw)
void _6502_pushw(_6502 *cpu, Mem *mem, uint32_t *cycles, WORD w)
{
    (cpu->s -= 2, memstw(mem, cycles, cpu->s + 2, w));
}

typedef enum {
    LD_IM = 0x0100,  // Load Immediate
    LD_ZP = 0x0101,  // Load (from) Zero Page
    LD_ZPX = 0x0102, // Load (from) Zero Page (+) X
} LDIns;

// http://www.6502.org/users/obelisk/6502/reference.html#LDA
#define LDA_IM  0xA9 // Load Accumulator Immediate                  // 2 cycles
#define LDA_ZP  0xA5 // Load Accumulator (from) Zero Page           // 3 cycles
#define LDA_ZPX 0xB5 // Load Acuumulatro (from) Zero Page (+) X     // 4 cycles
// http://www.6502.org/users/obelisk/6502/reference.html#LDX
#define LDX_IM 0xA2 // Load Accumulator Immediate                   // 2 cycles
#define LDX_ZP 0xA6 // Load Accumulator (from) Zero Page            // 3 cycles

// http://www.6502.org/users/obelisk/6502/reference.html#JSR
#define JSR 0x20 // Jump to Subroutine                              // 6 cycles

static uint16_t const insmap[0x100] = {
    [LDA_IM] = LD_IM,   //
    [LDA_ZP] = LD_ZP,   //
    [LDA_ZPX] = LD_ZPX, //
    [LDX_IM] = LD_IM,   //
    [LDX_ZP] = LD_ZP,   //
};

void _6502_ld(_6502 *cpu, Mem *mem, uint32_t *cycles, LDIns ld_instruction, BYTE *reg)
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
            *reg += *reg;
            if (*cycles == 0) {
                fprintf(stderr, "Not enough cycles to complete instruction\n");
                exit(1);
            }
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

void test_jsr_ldxim_zpx(void);

int main(void)
{
    test_jsr_ldxim_zpx();

    return 0;
}

void test_jsr_ldxim_zpx(void)
{
    Mem mem;
    _6502 cpu;
    _6502_reset(&cpu, &mem);

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

    mem.data[0x42] = 0x5;

    _6502_exec(&cpu, &mem, 18);

    assert(cpu.a == 0x5);
    assert(cpu.x == 0x10);

    printf("A = 0x%x\n", cpu.a); // 0x5
    printf("X = 0x%x\n", cpu.x); // 0x10
}

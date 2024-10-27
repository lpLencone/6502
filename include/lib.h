#ifndef _6502_LIB_H_
#define _6502_LIB_H_

#include <stdint.h>

typedef uint8_t BYTE;
typedef uint16_t WORD;

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

#endif // _6502_LIB_H_

BIN	:= 6502
SRC	:= $(shell find src -name "*.c")
INCLUDE	:= -Iinclude
CFLAGS	:= -Wall -Wextra -pedantic -ggdb

.PHONY: all test

all:
	gcc $(CFLAGS) $(INCLUDE) -o $(BIN) $(SRC)

test: tests/test.c
	gcc $(CFLAGS) $(INCLUDE) -o test.out tests/test.c src/mem.c src/6502.c
	./test.out; rm ./test.out

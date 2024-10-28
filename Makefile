BIN	:= 6502
SRC	:= $(shell find src -name "*.c")
INCLUDE	:= -Iinclude
CFLAGS	:= -Wall -Wextra -pedantic -ggdb -std=c23

.PHONY: all test

all:
	gcc $(CFLAGS) $(INCLUDE) -o $(BIN) $(SRC)

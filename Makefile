BIN	:= 6502
SRC	:= $(shell find src -name "*.c")
INCLUDE	:= -Iinclude
CFLAGS	:= -Wall -Wextra -pedantic -ggdb

all:
	gcc $(CFLAGS) $(INCLUDE) -o $(BIN) $(SRC)

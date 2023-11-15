CC ?= gcc
BIN := bfjit

CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -g -Og
all:
	$(CC) -o $(BIN) $(CFLAGS) $(wildcard src/*.c)

run: all
	./$(BIN) "tests/mandelbrot.bf"

clean:
	rm $(BIN) | true

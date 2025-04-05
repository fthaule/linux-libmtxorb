BIN = mtxorb_test

CFLAGS = -std=c99
CFLAGS += -Wall -Wextra -pedantic

CC = gcc
RM = rm

default: build

$(BIN): clean
	$(CC) $(CFLAGS) -I.. ../mtxorb.c test.c -o $@

.PHONY: build
build: $(BIN)

.PHONY: clean
clean:
	$(RM) -f $(BIN)

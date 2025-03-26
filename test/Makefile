BIN = mtxorb_test

CFLAGS = -std=c89
CFLAGS += -Wall -Wextra -Wpedantic

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

TARGET = mtxorb_test

CFLAGS = -std=c89 -pedantic
CFLAGS += -Wall -Wextra

CC = gcc
RM = rm

default: test

$(TARGET): clean
	$(CC) $(CFLAGS) -I.. ../mtxorb.c test.c -o $@

.PHONY: test
test: $(TARGET)
	./$(TARGET)

.PHONY: clean
clean:
	$(RM) -f $(TARGET)

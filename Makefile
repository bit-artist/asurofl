# asurofl - simple asuro flash tool
.POSIX:

CC = gcc
CFLAGS = -Os -pedantic -Wall
BIN = asurofl
OBJECTS = main.o util.o crc.o

all: $(BIN) size

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^

size: $(BIN)
	size $<

.c.o:
	$(CC) $(CFLAGS) -c $<

main.o: main.c util.h
util.o: util.c util.h crc.h
crc.o: crc.c crc.h

clean:
	rm -f $(BIN) $(OBJECTS)

.PHONY: all size clean

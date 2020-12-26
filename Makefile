CC = gcc
CFLAGS = -Os -c -pedantic -Wall
SRC = src
PROJECT = asurofl
OBJECT_FILES = main.o util.o crc.o

all: $(OBJECT_FILES)
	$(CC) -o $(PROJECT) $(OBJECT_FILES)
	size $(PROJECT)

main.o: $(SRC)/main.c $(SRC)/util.h
	$(CC) $(CFLAGS) -o main.o $(SRC)/main.c
	
util.o: $(SRC)/util.c $(SRC)/util.h $(SRC)/crc.h
	$(CC) $(CFLAGS) -o util.o $(SRC)/util.c

crc.o: $(SRC)/crc.c $(SRC)/crc.h
	$(CC) $(CFLAGS) -o crc.o $(SRC)/crc.c

clean:
	-rm $(PROJECT) $(OBJECT_FILES)


# targets that do not refer to files but are just actions
# are called phony targets
.PHONY: clean

CC=gcc
CFLAGS=-I.
LIBS=-lsqlite3

all: clean build

clean:
#	rm -f hash_table.db chinook_hash.db
	rm -f test

build:
	$(CC) -o main main.c $(LIBS)

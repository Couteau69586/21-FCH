CC = gcc
CFLAGS = -g -std=c99 -Wall

all: linker assembler

linker: linker.c my_assembler_utils.c my_linker_utils.c
	$(CC) $(CFLAGS) -o linker linker.c my_assembler_utils.c my_linker_utils.c -L. -lP7

assembler: assembler.c my_assembler_utils.c
	$(CC) $(CFLAGS) -o assembler assembler.c my_assembler_utils.c -L. -lP7

clean:
	rm -f *.o linker assembler

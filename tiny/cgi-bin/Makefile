CC = gcc
CFLAGS = -O2 -Wall -I ..

all: adder

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c -fsanitize=address

clean:
	rm -f adder *~

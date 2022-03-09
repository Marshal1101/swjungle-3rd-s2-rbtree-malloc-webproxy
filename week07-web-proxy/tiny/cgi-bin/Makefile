CC = gcc
CFLAGS = -O2 -Wall -I ..

all: adder form-adder

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c -fsanitize=address

form-adder.c: form-adder.c
	$(CC) $(CFLAGS) -o form-adder form-adder.c -fsanitize=address

clean:
	rm -f adder *~ form-adder *~

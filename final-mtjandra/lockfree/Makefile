CC = gcc
CFLAGS = -g -O0 -Wall -Werror
LDFLAGS = -lpthread


all: testcas testaccum


testcas: cmp_swap.o testcas.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

testaccum: cmp_swap.o accum.o testaccum.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f testcas testaccum *.o


.PHONY: all clean


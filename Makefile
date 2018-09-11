CC = g++ # clang++

all: mastermind

mastermind: mastermind.cc
	$(CC) --std=c++14 -I. -o mastermind -O3 mastermind.cc

mastermind.cc: mastermind.h

clean:
	rm -f *.o

realclean: clean
	rm -f mastermind


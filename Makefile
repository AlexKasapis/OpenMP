CC = gcc
CC1 = mpicc
CFLAGS = -Wall -g

all: clean generate examine
	
clean: 
	rm -f ./generator
	rm -f ./Examine
	rm -f *.o

generate: generator.c
	$(CC) $(CFLAGS) -o generator generator.c
	./generator datafile 15000000

examine: examine.c
	$(CC1) -fopenmp $(CFLAGS) -o examine Examine.c -std=gnu11
	./examine -1 -1 datafile -1 -1

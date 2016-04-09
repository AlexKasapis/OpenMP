CC = gcc
CC1 = mpicc
CFLAGS = -Wall -g

all: Generate Examine

Generate: generator.c
	$(CC) $(CFLAGS) -o generator generator.c
	./generator datafile 15000000

Examine: Examine.c Generate
	$(CC1) -fopenmp $(CFLAGS) -o Examine Examine.c -std=gnu11

clean: 
	rm ./generator
	rm ./Examine
	rm *.o

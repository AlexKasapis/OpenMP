CC = gcc
CC1 = mpicc
CFLAGS = -Wall -g

all: Generate Examine

Generate: generator.c
	$(CC) $(CFLAGS) -o generator generator.c
	./generator data 15000000



Examine: Examine.c Generate
	$(CC1) -fopenmp $(CFLAGS) -o Examine Examine.c
	./Examine -1 -1 data -1 -1


clean: 
	rm ./generator
	rm ./Examine
	rm *.o


CC= gcc
CFLAGS= -wall -g
INCLUDES= -I/path/to/custom/include
LIBS=-L/path/to/custom/lib

all: Examine Generator

Examine: Examine.o
    $(CC) $(LIBS) -o Examine

Generator: Generator.o
    $(CC) $(LIBS) -o Generator

Examine.o: Examine.c 
    $(CC) $(CFLAGS) $(INCLUDES) -c Examine.c

Generator.o: Generator.c 
    $(CC) $(CFLAGS) $(INCLUDES) -c Generator.c

clean:
    rm-f Generator Examine*.o core.*

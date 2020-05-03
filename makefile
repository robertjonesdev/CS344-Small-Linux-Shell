all: smallsh

smallsh: smallsh.o
	gcc -o smallsh smallsh.o

smallsh.o: smallsh.c
	gcc -c smallsh.c

clean:	
	rm smallsh.o

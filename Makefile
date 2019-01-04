microshell: microshell.o
	gcc -D_GNU_SOURCE -std=c99 -Wall microshell.o -o microshell.bin

microshell.o: microshell.c
	gcc -D_GNU_SOURCE -std=c99 -Wall -c microshell.c

clean:
	-rm *.o *.bin
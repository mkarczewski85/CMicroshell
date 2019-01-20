microshell: microshell.o
	gcc -D_GNU_SOURCE -std=c99 -Wall microshell.o -o microshell.bin

microshell.o: microshell.c
	gcc -D_GNU_SOURCE -std=c99 -Wall -c microshell.c

debug: microshell.c
	gcc -D_GNU_SOURCE -std=c99 -Wall microshell.c -g -o microshell.bin

clean:
	-rm *.o *.bin
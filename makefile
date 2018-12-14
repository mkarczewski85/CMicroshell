microshell: Tasks.o Interpret.o microshell.o
	gcc -D_GNU_SOURCE -std=c99 Tasks.o Interpret.o microshell.o -o microshell

Interpret.o : Interpret.h Interpret.c
	gcc -D_GNU_SOURCE -std=c99 -c Interpret.c

Tasks.o : Tasks.h Tasks.c
	gcc -D_GNU_SOURCE -std=c99 -c Tasks.h Tasks.c

microshell.o : microshell.c OutputUtility.h
	gcc -D_GNU_SOURCE -std=c99 -c microshell.c

clean:
	-rm *.o
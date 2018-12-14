#ifndef _MICROSHELL_INTERPRET_
#define _MICROSHELL_INTERPRET_

//Reads commands from stdin
char * Read();

//Parses command from Read()
char ** Parse(char command[]);

//Invokes given program with given arguments
int Invoke(char name[], char *args[]);

char ** _Tokenize(const char commands[]);

#endif // _MICROSHELL_INTERPRET_
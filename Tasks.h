#ifndef _MICROSHELL_TASKS_
#define _MICROSHELL_TASKS_

#include "OutputUtility.h"
#include <string.h>
#include <malloc.h>
#include "System.h"

struct Path
{
	int maxLength;
	int count;
};

typedef struct Path path;

void Prompt(const char user[], const char computer[], const char path[], const char prompt);

char ** ParsePATH(const char PATH[]);

char ** ParseCommand(const char Command[]);

char * GetPATH();

path _PathMaxLength(const char PATH[]);

#endif // _MICROSHELL_TASKS_;

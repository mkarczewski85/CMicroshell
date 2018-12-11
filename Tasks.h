#ifndef _TASKS_
#define _TASKS_

#include "OutputUtility.h"
#include <string.h>
#include <malloc.h>

struct Path
{
	int maxLength;
	int count;
};

typedef struct Path path;

void Prompt(const char path[], const char prompt);

char ** ParsePATH(const char PATH[]);

char ** ParseCommand(const char Command[]);

path _PathMaxLength(const char PATH[]);

#endif // _TASKS_;

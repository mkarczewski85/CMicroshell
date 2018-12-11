#include "OutputUtility.h"
#include "Tasks.h"

//***Boolean types***
#ifndef _BOOL_

#define true 1
#define false 0

#endif // _BOOL_
//-------------------

#define BUFF_SIZE 128

int main()
{
	char args[BUFF_SIZE];
	char **paths = NULL;
	int i = 0;
	const char *PATH = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/home/jakub/Pulpit/balnxxe";

	paths = ParsePATH(PATH);

	return 0;
}
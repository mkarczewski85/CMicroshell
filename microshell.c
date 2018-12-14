#include "OutputUtility.h"
#include "Tasks.h"
#include "Interpret.h"
#include "System.h"
#include <string.h>

//-------------------------------------Boolean types----------------------------------------------
#ifndef _BOOL_

#define true 1
#define false 0

#endif // _BOOL_
//------------------------------------------------------------------------------------------------

//-----------------------------------------LIMITS-------------------------------------------------
#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX 65
#endif // LOGIN_NAME_MAX

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif // HOST_NAME_MAX
//------------------------------------------------------------------------------------------------
#define BUFF_SIZE 128

int main()
{
	char **paths = NULL;
	char username[LOGIN_NAME_MAX];
	char hostname[HOST_NAME_MAX];
	char c;
	char *command = NULL;
	gethostname(hostname, HOST_NAME_MAX);
	getlogin_r(username, LOGIN_NAME_MAX);
	int i = 0;

	command = Read();
	if(command == NULL)
	{
		printf("We have an error here!\n");
	}
	

	return 0;
}
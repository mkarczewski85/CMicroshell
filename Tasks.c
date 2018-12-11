#include "Tasks.h"

void Prompt(const char path[], const char prompt)
{
	printf(COLOR_FOLDER "%c%s" COLOR_RESET "%c", '~', path, prompt);
	fflush(stdout);
}

char ** ParsePATH(const char PATH[])
{
	///-------------------------------------Variables----------------------------------------
	///
	/// @var _pathInfo is path object holding maximal path length and _paths count
	///
	const path _pathInfo = _PathMaxLength(PATH);
	int i = 0, j = 0, k = 0;
	char current = EOF;
	const int length = strlen(PATH);
	///--------------------------------------------------------------------------------------

	///------------------------------Allocating string array---------------------------------
	char **_paths = malloc(_pathInfo.count * sizeof(char*));
	for(i = 0; i < _pathInfo.count; i++)
	{
		_paths[i] = malloc(_pathInfo.maxLength * sizeof(char));
	}
	//--------------------------------------------------------------------------------------

	for(i = 0; i < _pathInfo.count; i++)
	{
		for(k = 0; j < length; k++)
		{
			current = PATH[j];
			j++;
			if(current != ':')	//if reads ':' ends string and goes to next loop iteration
			{
				_paths[i][k] = current;
			}
			else
			{
				_paths[i][k] = '\0';
				break;
			}
		}
	}

	return _paths;
}

char ** ParseCommand(const char Command[])
{

}

path _PathMaxLength(const char PATH[])
{
	char current = EOF;
	path res = {0, 0};
	int i = 0, curLength = 0, maxLength = 0;
	for(i = 0; i < strlen(PATH); i++)
	{
		current = PATH[i];
		if(current == ':')
		{
			if(curLength > maxLength)
			{
				maxLength = curLength;
			}
			curLength = 0;
			res.count++;
		}
		else
			curLength++;
	}
	if(curLength > maxLength)
		maxLength = curLength;
	res.maxLength = maxLength;
	res.count++;
	
	return res;
}
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <wait.h>
#include <error.h>
#include <errno.h>

#define COLOR_FOLDER "\x1B[1;34m"
#define COLOR_USER "\x1B[1;92m"
#define COLOR_RESET "\x1B[0m"
#define COLOR_HIDDEN "\x1B[1;90m"
#define COLOR_IMPORTANT "\x1B[1;35m"

#define CLEAR_SCREEN "\x1B[2;J"

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX 65
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define _SUCCESS 0
#define _FAIL -1

#define _STRING_WORD_SIZE 255

typedef char* string;

struct Command
{
	string name;
	string args[];
};

typedef struct Command command;

enum Bool
{
	false = 0,
	true = 1
};

typedef enum Bool bool;

/* Allocates memory for command with argsCount arguments */
command *_commalloc(const int argsCount);

/* Deallocates memory allocated by _commalloc for command with argsCount arguments. */
int _commfree(command *_source, int argsCount);

/* Displays prompt for user containing username, computername, current path and prompt symbol. */
int Prompt(string _user, string _host, const char symbol);

/* Reads commands from stdin */
int Read(string comm);

/* Parses _parsed to command */
command *Parse(string _parsed);

/* Invokes passed command if possible. */
int Invoke(command *comm);

int exec_extern(const command *comm);

/* Helper function counting items divided by separator in string */
int _ItemsCount(const string _items, const char _delimeters[]);

string __trimaround(const string _source);

string *_advtok(const string _source, const char _delimeters[], int *_elements);

int microshell_cd(char *args[]);

int microshell_exit(char *args[]);

int microshell_help(char *args[]);

string built_in_names[] = 
{
	"cd",
	"help",
	"exit"
};

int (*built_in[])(char *args[]) = 
{
	&microshell_cd,
	&microshell_help,
	&microshell_exit
};

int built_in_num()
{
	return (sizeof(built_in_names)/sizeof(string));
}

int main()
{
	string username = NULL, hostname = NULL, input = NULL;
	command *comm = NULL;

	username = calloc(LOGIN_NAME_MAX + 1, sizeof(char));
	hostname = calloc(HOST_NAME_MAX + 1, sizeof(char));
	input = calloc(PATH_MAX + 1, sizeof(char));
	if(!username || !hostname || !input)
		perror("Cannot allocate memory");

	while(1)
	{
		Prompt(username, hostname, '$');
		Read(input);
		comm = Parse(__trimaround(input));
		if(!comm)
			continue;
		Invoke(comm);
		memset(username, 0, LOGIN_NAME_MAX);
		memset(hostname, 0, HOST_NAME_MAX);
		memset(input, 0, PATH_MAX);
	}

	free(username);
	free(hostname);
	fputc('\n', stdout);

	return 0;
}

command *_commalloc(const int argsCount)
{
	int i = 0;
	command *comm = (command*)malloc(sizeof(command) + argsCount * sizeof(string));

	comm->name = malloc((_STRING_WORD_SIZE + 1) * sizeof(char));

	if(!argsCount)
		return comm;

	for(i = 0; i < argsCount; i++)
	{
		comm->args[i] = malloc((_STRING_WORD_SIZE + 1) * sizeof(char));
		if(!(comm->args[i]))
			return NULL;
	}

	if(!comm)
		return NULL;

	return comm;
}

int _commfree(command *comm, const int argsCount)
{
	int i = 0;

	free(comm->name);
	comm->name = NULL;

	for(i = 0; i < argsCount; i++)
	{
		free(comm->args[i]);
		comm->args[i] = NULL;
	}

	free(comm);
	comm = NULL;
	perror("After freeing comm");
	return _SUCCESS;
}

int Prompt(string _user, string _host, const char symbol)
{
	errno = 0;
	char path[PATH_MAX];
	getcwd(path, PATH_MAX);
	gethostname(_host, HOST_NAME_MAX);
	getlogin_r(_user, LOGIN_NAME_MAX);
	printf(COLOR_USER "%s@%s:" COLOR_FOLDER "~%s" COLOR_RESET "%c ", _user, _host, path, symbol);
	fflush(stdout);
	if(errno)
	{
		perror("Cannot get host or user name");
		return _FAIL;
	}
	return _SUCCESS;
}

int Read(string comm)
{
	int current = -5;
	char _tmp[2] = {0, 0};
	while((current = fgetc(stdin)))
	{
		if((char)current == '\n' || current == EOF)
		{
			strcat(comm, "");
			return _SUCCESS;
		}
		else
		{
			_tmp[0] = (char)current;
			strcat(comm, _tmp);
		}
	}
	return _FAIL;
}

command *Parse(string _parsed)
{
	int elements,
		iter;
	string *commands;
	command *_comm = NULL;
	commands = _advtok(_parsed, "\'\"", &elements);
	if(!elements)
	{
		free(commands);
		return NULL;
	}
	_comm = _commalloc(elements + 1);
	_comm->name = commands[0];

	for(iter = 0; iter < elements; iter++)
	{
		if(strlen(commands[iter]) + 1 >= _STRING_WORD_SIZE)
		{
			_comm->args[iter] = realloc(_comm->args[iter], (strlen(commands[iter]) + 1) * sizeof(char));
			printf("Test\n");
			if(!_comm->args[iter])
			{
				return (command*)_FAIL;
			}
			memset(_comm->args[iter], 0, strlen(commands[iter]) + 1);
		}
		if(!strcpy(_comm->args[iter], commands[iter]))
			return (command*)_FAIL;
	}
	_comm->args[elements] = NULL;
	free(commands);
	return _comm;
}

int Invoke(command *comm)
{
	int i, num = built_in_num();
	for(i = 0; i < num; i++)
	{
		if(!strcmp(comm->name, built_in_names[i]))
			return (*built_in[i])(comm->args);
	}

	return exec_extern(comm);
}

int exec_extern(const command *comm)
{
	int status = 0, check = 0;
	pid_t pid = -1;
	pid = fork();

	/* Error */
	if(pid < 0)
	{
		perror("Fork error");
	}
	/* Child */
	else if(pid == 0)
	{
		check = execvp(comm->name, comm->args);
		
		if(check == -1)
			perror("Execution error");
	}
	else
	{
		waitpid(pid, &status, WUNTRACED);
	}

	return status;
}

int _ItemsCount(const string _items, const char _delimeters[])
{
    int length = strlen(_items), _count = 0, i = 0;
    char current = 0;
    for(i = 0; i < length; i++)
    {
        current = _items[i];
        if(strchr(_delimeters, current) != NULL)
            _count++;
    }
    return ++_count;
}

/* Advanced Tokenize divides _source into parts divided by delimeter if not quoted in _quotes */
string *_advtok(const string _source, const char _quotes[], int *_elements)
{
	const char _delimeters[] = {' ', '\t', '\r', '\n', '\v', '\f', '\0'};

	char *_current = _source,
		 *_isQuote = NULL,
		 *_lastQuote = NULL,
		 *_isWhitespace = NULL;

	bool inside = false,
		 isWord = false;

	int _elems = 1,
		_outPos = 0,
		_elemLength = _STRING_WORD_SIZE,
		_writingPos = 0,
		_iter = 0,
		_length = strlen(_source) + 1;
	
	string _tmp = calloc(_elemLength + 1, sizeof(char)),
		   *_out = calloc(_elems, sizeof(string));

	for(_iter = 0, _outPos = 0, _writingPos = 0; _iter < _length; _iter++, _current++)
	{
		_isQuote = strchr(_quotes, *_current);
		_isWhitespace = strchr(_delimeters, *_current);
		if(inside)
		{
			if((_isQuote != NULL) && (_isQuote == _lastQuote))
			{
				inside = false;
				_lastQuote = NULL;
				_out[_outPos] = calloc(strlen(_tmp) + 1, sizeof(char));
				strcpy(_out[_outPos], _tmp);
				_elems++;
				_outPos++;
				_out = realloc(_out, _elems * sizeof(string));
				if(!_out)
				{
					free(_tmp);
					return NULL;
				}
				_writingPos = 0;
				_elemLength = _STRING_WORD_SIZE;
				_tmp = realloc(_tmp, (_elemLength + 1) * sizeof(char));
				if(!_tmp)
				{
					perror("_tmp allocation error");
					return NULL;
				}
				memset(_tmp, 0, _elemLength + 1);
			}
			else
			{
				if(_writingPos >= _elemLength)
				{
					_elemLength += _STRING_WORD_SIZE;
					_tmp = realloc(_tmp, (_elemLength + 1) * sizeof(char));
					if(!_tmp)
					{
						perror("noQuote _tmp allocation error");
						return NULL;
					}
				}
				_tmp[_writingPos] = *_current;
				_writingPos++;
			}
		}
		else
		{
			if(_isQuote != NULL)
			{
				if(isWord)
				{
					isWord = false;
					_out[_outPos] = calloc(strlen(_tmp) + 1, sizeof(char));
					strcpy(_out[_outPos], _tmp);
					_elems++;
					_outPos++;
					_out = realloc(_out, _elems * sizeof(string));
					if(!_out)
					{
						free(_tmp);
						return NULL;
					}
					_writingPos = 0;
					_elemLength = _STRING_WORD_SIZE;
					_tmp = realloc(_tmp, (_elemLength + 1) * sizeof(char));
					if(!_tmp)
					{
						perror("_tmp allocation error");
						return NULL;
					}
					memset(_tmp, 0, _elemLength + 1);
				}
				_lastQuote = _isQuote;
				inside = true;
			}
			else if((_isWhitespace != NULL))
			{
				if(isWord)
				{
					isWord = false;
					_out[_outPos] = calloc(strlen(_tmp) + 1, sizeof(char));
					strcpy(_out[_outPos], _tmp);
					_elems++;
					_outPos++;
					_out = realloc(_out, _elems * sizeof(string));
					if(!_out)
					{
						free(_tmp);
						return NULL;
					}
					_writingPos = 0;
					_elemLength = _STRING_WORD_SIZE;
					_tmp = realloc(_tmp, (_elemLength + 1) * sizeof(char));
					if(!_tmp)
					{
						perror("_tmp allocation error");
						return NULL;
					}
					memset(_tmp, 0, _elemLength + 1);
				}
			}
			else
			{
				isWord = true;
				if(_writingPos >= _elemLength)
				{
					_elemLength += _STRING_WORD_SIZE;
					_tmp = realloc(_tmp, (_elemLength + 1) * sizeof(char));
					if(!_tmp)
					{
						perror("noQuote _tmp allocation error");
						return NULL;
					}
				}
				_tmp[_writingPos] = *_current;
				_writingPos++;
			}
		}	
	}
	free(_tmp);
	*_elements = _elems - 1;
	return _out;
}

/* Trim white spaces before first and after last character. */
string __trimaround(const string _source)
{
	const char _delimeters[] = {' ', '\t', '\r', '\n', '\v', '\f'};

	int pos = 0,
		length = strlen(_source),
		bef = 0, aft = 0;
	
	char *current = _source,
		 *res = calloc(length + 1, sizeof(char));
	
	while((strchr(_delimeters, *current) != NULL) && (pos <= length))
	{
		pos++;
		current++;
	}

	bef = pos;
	if(bef == length)
		return NULL;

	if(bef < length)
	{
		current = &_source[length - 1];
		pos = 0;

		while((strchr(_delimeters, *current) != NULL) && (pos <= length))
		{
			pos++;
			current--;
		}

		aft = pos;
	}
	current = &_source[bef];
	for(pos = 0; pos < (length - (bef + aft)); pos++, current++)
	{
		res[pos] = *current;
	}
	return res;
}

int microshell_cd(char *args[])
{
	int status = 0;

	if(!args[1])
	{
		printf("Error: no arguments for cd");
		return _FAIL;
	}
	status = chdir(args[1]);
	if(status == -1)
	{
		perror("Error");
		errno = 0;
		return _FAIL;
	}
	return _SUCCESS;
}

int microshell_exit(char *args[])
{
	exit(EXIT_SUCCESS);
	return 0;
}

int microshell_help(char *args[])
{
	printf(CLEAR_SCREEN);
	printf("\x1B[90m" "\t\t\t\tMicroshell (2018)\n" COLOR_RESET);
	printf(COLOR_IMPORTANT "Author: " COLOR_RESET "Jakub Kwiatkowski\n" COLOR_RESET);
	
	return _SUCCESS;
}
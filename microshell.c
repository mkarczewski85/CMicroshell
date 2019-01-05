#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#define COLOR_FILE "\x1B[37m"
#define COLOR_FOLDER "\x1B[1;34m"
#define COLOR_LINK "\x1B[36m"
#define COLOR_DEVICE "\x1B[33m"
#define COLOR_USER "\x1B[1;92m"
#define COLOR_RESET "\x1B[0m"
#define COLOR_HIDDEN "\x1B[1;90m"
#define COLOR_IMPORTANT "\x1B[1;35m"
#define FONT_BOLD "\x1B[1m"

#define CLEAR_SCREEN "\x1B[0;0H\x1B[2;J"

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

int microshell_ls(char *args[]);

int microshell_touch(char *args[]);

string built_in_names[] = 
{
	"cd",
	"help",
	"exit",
	"ls",
	"touch"
};

int (*built_in[])(char *args[]) = 
{
	&microshell_cd,
	&microshell_help,
	&microshell_exit,
	&microshell_ls,
	&microshell_touch
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
	string path = calloc(PATH_MAX + 1, sizeof(char));
	char *sub = NULL;

	if(!args[1])
	{
		printf("Error: no arguments for cd");
		return _FAIL;
	}
	if((sub = strstr(args[1], "~/")) != NULL)
	{
		sub += 2;
		path = strcat(path, getenv("HOME"));
		path = strcat(path, "/");
		path = strcat(path, sub);
		printf("%s\n", path);
	}
	else
	{
		path = strcpy(path, args[1]);
	}
	status = chdir(path);
	if(status == -1)
	{
		perror("Error");
		errno = 0;
		return _FAIL;
	}
	free(path);
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
	printf("\x1B[29G");
	printf(COLOR_IMPORTANT "Microshell (2018/2019)\n" COLOR_RESET);
	printf(COLOR_HIDDEN "Author: " COLOR_RESET "Jakub Kwiatkowski" "\x1B[61G" COLOR_HIDDEN "version: " COLOR_RESET "2019.01.05\n");
	printf("This Microshell is simple Linux/Unix shell. \nIt allows you to execute programs right here in command line.\n");
	printf("Arguments can be quoted in" FONT_BOLD " \"\" or \'\'" COLOR_RESET ". Type " FONT_BOLD "exit" COLOR_RESET " to exit microshell.\nEnjoy your new supertool!\n");

	return _SUCCESS;
}

void microshell_ls_help()
{
	printf(CLEAR_SCREEN);
	printf(FONT_BOLD "Microshell ls help.\n" COLOR_RESET);
	printf(COLOR_IMPORTANT "Command:\n" COLOR_RESET "\tls [path] [options]\n");
	printf(FONT_BOLD "Options:\n" COLOR_RESET);
	printf("\t-a | display all content (including files started with .)\n");
	printf("\t-l | display long info (user | group | size | modification date | name)\n");
	printf("\t-q | display names in double quotes\n");
	printf("\t-h | display this help\n");
}

string __getnamefromUID(uid_t UID)
{
	struct passwd *pwd = getpwuid(UID);
	if(!pwd)
	{
		perror("User not found");
		return NULL;
	}
	string _out = pwd->pw_name;
	return _out;
}

string __getnamefromGID(gid_t GID)
{
	struct group *grp = getgrgid(GID);
	if(!grp)
	{
		perror("Group not found");
		return NULL;
	}
	string _out = grp->gr_name;
	return _out;
}

int microshell_ls(char *args[])
{
	const char _delimeters[] = {' ', '\t', '\r', '\n', '\v', '\f'};

	string path, filepath = calloc(PATH_MAX + 1, sizeof(char));
	string COLOR  = COLOR_FILE;
	string format = calloc(256, sizeof(char));
	struct tm *mod_date;
	DIR *directory;
	struct dirent *entry;
	struct stat entry_stat;
	bool whitespace_name = false;

	struct _disp
	{
		bool l;
		bool a;
		bool q;
	}display = {false, false, false};

	struct _props
	{
		string name;
		string user;
		string group;
		char modtime[24];
		off_t size;

	}fprops;

	string _args[] =
	{
		"-l",
		"-a",
		"-q",
		"-h"
	};

	int i = 0, j = 0;
	while(args[i] != NULL)
	{
		for(j = 0; j < (sizeof(_args)/sizeof(string)); j++)
		{
			if(!strcmp(args[i], _args[j]))
			{
				switch(j)
				{
					case 0:
						display.l = true;
						break;
					case 1:
						display.a = true;
						break;
					case 2:
						display.q = true;
						break;
					case 3:
						microshell_ls_help();
						break;
				}
			}
		}
		i++;
	}

	path = args[1];
	if(!path)
		path = getcwd(NULL, 0);
	if(!(directory = opendir(path)))
	{
		path = getcwd(NULL, 0);
		if(!path)
		{
			perror("Cannot open current directory");
			return _FAIL;
		}
		else if(!(directory = opendir(path)))
		{
			perror("Cannot open current directory");
			return _FAIL;
		}
	}

	while((entry = readdir(directory)) != NULL)
	{
		if((entry->d_name[0] == '.') && (!display.a))
			continue;
		for(i = 0; i < (sizeof(_delimeters)/sizeof(char)); i++)
		{
			if(strchr(entry->d_name, _delimeters[i]) != NULL)
			{
				whitespace_name = true;
				break;
			}
			else
				whitespace_name = false;
		}
		filepath = strcat(filepath, path);
		filepath = strcat(filepath, "/");
		filepath = strcat(filepath, entry->d_name);
		if(stat(filepath, &entry_stat) == -1)
		{
			perror("Cannot read file properties");
			printf("%s\n", entry->d_name);
			continue;
		}
		
		if(S_ISREG(entry_stat.st_mode))
			COLOR = COLOR_FILE;
		else if(S_ISDIR(entry_stat.st_mode))
			COLOR = COLOR_FOLDER;
		else if(S_ISLNK(entry_stat.st_mode))
			COLOR = COLOR_LINK;
		else if(S_ISBLK(entry_stat.st_mode))
			COLOR = COLOR_DEVICE;

		if(display.l)
		{
			format = strcat(format, "%s %s\t%d\t%.24s\t");
			format = strcat(format, COLOR);
			format = (display.q || whitespace_name) ? strcat(format, "\"%s\"\n") : strcat(format, "%s\n");
			format = strcat(format, COLOR_RESET);

			fprops.user = __getnamefromUID(entry_stat.st_uid);
			fprops.group = __getnamefromGID(entry_stat.st_gid);
			fprops.size = entry_stat.st_size;
			mod_date = localtime(&entry_stat.st_mtim.tv_sec);
			strftime(fprops.modtime, 24, "%a %Y/%m/%d %T", mod_date);
			fprops.name = entry->d_name;

			printf(format, fprops.user, fprops.group, fprops.size, fprops.modtime, fprops.name);
		}
		else
		{
			format = strcat(format, COLOR);
			format = (display.q || whitespace_name) ? strcat(format, "\"%s\"\n") : strcat(format, "%s\n");
			format = strcat(format, COLOR_RESET);

			printf(format, entry->d_name);
		}

		memset(format, 0, 256);
		memset(filepath, 0, PATH_MAX + 1);
	}

	free(format);
	free(filepath);
	return _SUCCESS;
}

void microshell_touch_help()
{
	printf(CLEAR_SCREEN);
	printf(FONT_BOLD "Microshell touch help.\n" COLOR_RESET);
	printf(COLOR_IMPORTANT "Command:\n" COLOR_RESET "\ttouch [path] [-h][content]\n");
	printf(FONT_BOLD "Options:\n" COLOR_RESET);
	printf("\t-h | display this help\n");
	printf(FONT_BOLD "Path:\n" COLOR_RESET);
	printf("\tIf contains only filename file will be created in current directory\n");
	printf("\telse (if possible) file will be created in given folder.\n");
	printf(FONT_BOLD "Content:\n" COLOR_RESET);
	printf("\tOptional content of created file. Have to be quoted in \"\" or \'\'\n");
	printf("\tand cannot contain same quotes inside e.g. you can type \"My 'new' text\"\n");
	printf("\tbut cannot \"My \"new\" text\".\n");
}

int microshell_touch(char *args[])
{
	int fd, status, i;
	if(!args[1])
	{
		perror("No filename");
		return _FAIL;
	}

	if(!strcmp(args[1], "-h"))
	{
		microshell_touch_help();
		return _SUCCESS;
	}

	fd = open(args[1], O_WRONLY | O_CREAT | O_EXCL, 0666);
	if(fd == -1)
	{
		perror("Cannot create file");
		return _FAIL;
	}
	printf("Should've been written: %s\n", args[2]);
	if(args[2] != NULL)
		for(i = 0; i < strlen(args[2]); i++)
		{
			if((status = write(fd, &args[2][i], 1)) == -1)
			{
				perror("Cannot write to file");
			}
		}
		
	status = close(fd);
	if(status == -1)
	{
		perror("Cannot close file");
		return _FAIL;
	}
	return _SUCCESS;
}
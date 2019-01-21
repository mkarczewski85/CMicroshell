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

typedef struct Command
{
	size_t elements;
	char name[PATH_MAX];
	string args[256];
}Command;

typedef enum Bool
{
	false = 0,
	true = 1
}bool;

/* Allocates memory for Command with argsCount arguments */
int _commalloc(Command *comm, const int argsCount);

/* Deallocates memory allocated by _commalloc for Command with argsCount arguments. */
int _commfree(Command *_source);

/* Displays prompt for user containing username, computername, current path and prompt symbol. */
int Prompt(string _user, string _host, const char symbol);

/* Reads commands from stdin */
int Read(string comm);

/* Parses _parsed to Command */
int Parse(string _parsed, Command *_dest);

/* Invokes passed Command if possible. */
int Invoke(Command comm);

/* Helper function that invokes external program */
int exec_extern(const Command comm);

/* Helper function searching in PATH for given program */
string _lookforinPATH(string name);

/* Helper function that trim whitespaces around string */
string __trimaround(const string _source);

/* Advanced Tokenize divides _source into parts divided by delimeter if not quoted in _quotes */
string *_advtok(const string _source, const char _delimeters[], int *_elements, string delimeters);

/* Built-in functions */

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
	Command comm;

	username = calloc(LOGIN_NAME_MAX + 1, sizeof(char));
	hostname = calloc(HOST_NAME_MAX + 1, sizeof(char));
	input = calloc(PATH_MAX + 1, sizeof(char));
	if(!username || !hostname || !input)
	{
		perror("Error");
		goto END;
	}

	while(true)
	{
		Prompt(username, hostname, '$');
		Read(input);
		if(Parse(__trimaround(input), &comm) == _FAIL)
		{
			continue;
		}
		Invoke(comm);
		memset(username, 0, LOGIN_NAME_MAX);
		memset(hostname, 0, HOST_NAME_MAX);
		memset(input, 0, PATH_MAX);
		_commfree(&comm);
	}

	END:
	free(username);
	free(hostname);
	free(input);
	fputc('\n', stdout);

	return 0;
}

int _commalloc(Command *comm, const int argsCount)
{
	int i = 0;

	if(argsCount > 255)
	{
		printf("Too many arguments. Limit is 255\n");
		return _FAIL;
	}

	comm->elements = argsCount;

	if(!argsCount)
		return _SUCCESS;

	for(i = 0; i < argsCount; i++)
	{
		comm->args[i] = malloc((_STRING_WORD_SIZE + 1) * sizeof(char));
		if(!(comm->args[i]))
		{
			perror("Allocation error occured");
			return _FAIL;
		}
	}

	return _SUCCESS;
}

int _commfree(Command *_source)
{
	errno = 0;
	int i = 0;
	memset(_source->name, 0, sizeof(_source->name));
	for(i = 0; i < _source->elements; i++)
	{
		free(_source->args[i]);
		_source->args[i] = NULL;
	}

	if(errno)
		perror("Error");
	return _SUCCESS;
}

int Prompt(string _user, string _host, const char symbol)
{
	errno = 0;
	char path[PATH_MAX + 1];
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
	errno = 0;
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
	if(errno)
		perror("Error");
	return _FAIL;
}

int Parse(string _parsed, Command *_dest)
{
	errno = 0;
	int elements,
		iter;
	string *commands;
	commands = _advtok(_parsed, "\'\"", &elements, "");
	if(!elements)
	{
		free(commands);
		return _FAIL;
	}
	if(_commalloc(_dest, elements) == _FAIL)
	{
		perror("Error");
	}
	strcpy(_dest->name, commands[0]);

	for(iter = 0; iter < elements; iter++)
	{
		if(strlen(commands[iter]) + 1 >= _STRING_WORD_SIZE)
		{
			_dest->args[iter] = realloc(_dest->args[iter], (strlen(commands[iter]) + 1) * sizeof(char));
			if(!_dest->args[iter])
			{
				perror("Error");
				free(commands);
				return _FAIL;
			}
			memset(_dest->args[iter], 0, strlen(commands[iter]) + 1);
		}
		if(!strcpy(_dest->args[iter], commands[iter]))
		{
			perror("Error");
			free(commands);
			return _FAIL;
		}
	}
	_dest->args[elements] = NULL;
	free(commands);
	if(errno)
		perror("Error");
	return _SUCCESS;
}

int Invoke(Command comm)
{
	string _tmp = NULL;
	int i, num = built_in_num();
	for(i = 0; i < num; i++)
	{
		if(!strcmp(comm.name, built_in_names[i]))
			return (*built_in[i])(comm.args);
	}

	_tmp = _lookforinPATH(comm.name);
	if(_tmp != NULL)
		strcpy(comm.name, _tmp);
	free(_tmp);

	return exec_extern(comm);
}

int exec_extern(const Command comm)
{
	errno = 0;
	int status = 0, check = 0;
	pid_t pid = -1;
	pid = fork();

	/* Error */
	if(pid < 0)
	{
		perror("Fork error");
		return _FAIL;
	}
	/* Child */
	else if(pid == 0)
	{
		check = execv(comm.name, comm.args);
		
		if(check == -1)
		{
			perror("Execution error");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		waitpid(pid, &status, WUNTRACED);
	}
	if(errno)
		perror("Error");
	return status;
}

string _lookforinPATH(string name)
{
	string cwd = calloc(PATH_MAX, sizeof(char));
	getcwd(cwd, PATH_MAX - 256);
	if(name[0] == '/')
		return name;
	else if(strstr(name, "./"))
	{
		strcat(cwd, name + 1);
		return cwd;
	}
	DIR *directory;
	struct dirent *entry;
	string PATH = getenv("PATH"),
		   *token;
	int elements, i;
	string _ret = calloc(PATH_MAX, sizeof(char));
	token = _advtok(PATH, "", &elements, ":");
	for(i = 0; i < elements; i++)
	{
		if(!(directory = opendir(strcat(token[i], "/"))))
		{
			perror("Error");
			return NULL;
		}
		while((entry = readdir(directory)))
		{
			if(!strcmp(name, entry->d_name))
			{
				strcat(_ret, token[i]);
				strcat(_ret, name);
				return _ret;
			}
		}
	}
	return NULL;
}

string *_advtok(const string _source, const char _quotes[], int *_elements, string _delimeters)
{
	char _delim[] = {' ', '\t', '\r', '\n', '\v', '\f', '\0'};

	if(!strcmp(_delimeters, ""))
		_delimeters = _delim;
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

string __trimaround(const string _source)
{
	errno = 0;
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
	if(errno)
	{
		free(res);
		perror("Error");
		return NULL;
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
		printf("Error: no arguments for cd\n");
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
	if(status == -1 || errno)
	{
		free(path);
		perror("Error");
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
	printf(COLOR_HIDDEN "Author: " COLOR_RESET "Jakub Kwiatkowski" "\x1B[61G" COLOR_HIDDEN "version: " COLOR_RESET "2019.01.10\n");
	printf("This Microshell is simple Linux/Unix shell. \nIt allows you to execute programs right here in Command line.\n");
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
	errno = 0;
	const char _delimeters[] = {' ', '\t', '\r', '\n', '\v', '\f'};

	string path = calloc(PATH_MAX - 256, sizeof(char)), filepath = calloc(PATH_MAX + 1, sizeof(char));
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
						return _SUCCESS;
				}
			}
		}
		i++;
	}

	char *sub = NULL;

	if(!args[1])
		path = getcwd(NULL, 0);
	else if((sub = strstr(args[1], "~/")) != NULL)
	{
		sub += 2;
		path = strcat(path, getenv("HOME"));
		path = strcat(path, "/");
		path = strcat(path, sub);
	}
	else
		path = strncpy(path, args[1], PATH_MAX - 257);

	if(!(directory = opendir(path)))
	{
		errno = 0;
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
		filepath = strncat(filepath, path, PATH_MAX - 257);
		filepath = strncat(filepath, "/", 1);
		filepath = strncat(filepath, entry->d_name, 256);
		if(stat(filepath, &entry_stat) == -1)
		{
			perror("Cannot read file properties");
			errno = 0;
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
			format = strncat(format, COLOR, 118);
			format = (display.q || whitespace_name) ? strcat(format, "\"%s\"\n") : strcat(format, "%s\n");
			format = strncat(format, COLOR_RESET, 118);

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
			format = strncat(format, COLOR, 125);
			format = (display.q || whitespace_name) ? strcat(format, "\"%s\"\n") : strcat(format, "%s\n");
			format = strncat(format, COLOR_RESET, 125);

			printf(format, entry->d_name);
		}

		memset(format, 0, 256);
		memset(filepath, 0, PATH_MAX + 1);
	}
	closedir(directory);
	free(format);
	free(path);
	free(filepath);
	if(errno)
		perror("Error");
	return _SUCCESS;
}

void microshell_touch_help()
{
	printf(CLEAR_SCREEN);
	printf(FONT_BOLD "Microshell touch help.\n" COLOR_RESET);
	printf(COLOR_IMPORTANT "Command:\n" COLOR_RESET "\ttouch [path]/[-h]\n");
	printf("\tCreate or modify files\n");
	printf(FONT_BOLD "Options:\n" COLOR_RESET);
	printf("\t-h | display this help\n");
	printf(FONT_BOLD "Path:\n" COLOR_RESET);
	printf("\tIf contains only filename file will be created in current directory\n");
	printf("\telse (if possible) file will be created in given folder.\n");
	printf("\tIf file already exist changes its modification date.\n");
}

int microshell_touch(char *args[])
{
	errno = 0;
	int fd, status, i = 1;
	time_t t;
	struct timespec ts[2];

	while(args[i] != NULL)
	{
		if(!strcmp(args[i], "-h"))
		{
			microshell_touch_help();
			return _SUCCESS;
		}
		else
		{
			fd = open(args[i], O_RDONLY | O_APPEND | O_CREAT, 0666);
			if(fd == -1)
			{
				perror("Cannot create or open file");
				return _FAIL;
			}
			ts[0].tv_sec = ts[1].tv_sec = time(&t);
			if(futimens(fd, ts) == -1)
				perror("Cannot change file properties");
			memset(ts, 0, sizeof(ts));
			status = close(fd);
			if(status == -1)
			{
				perror("Cannot close file");
				return _FAIL;
			}
		}
		i++;
	}
	
	if(errno)
		perror("Error");
	return _SUCCESS;
}
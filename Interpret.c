#include "Interpret.h"
#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

const int _READING_BUFFER_SIZE = 1024;
const int _MAX_WORD_SIZE = 64;

char * Read()
{
    int status = 1, position = 0, buffer_size = _READING_BUFFER_SIZE;
    char current = '\0';
    char *temp = malloc(buffer_size * sizeof(char));
    if(!temp)
    {
        perror("Allocation error");
        return NULL;
    }
    while(status)
    {
        current = getchar();
        if(current == '\n' || current == EOF)
        {
            temp[position] = '\0';
            return temp;
        }
        temp[position] = current;
        position++;

        if(position >= _READING_BUFFER_SIZE)
        {
            buffer_size += _READING_BUFFER_SIZE;
            temp = realloc(temp, buffer_size * sizeof(char));

            if(!temp)
            {
                perror("Reallocation error");
                return NULL;
            }
        }
    }
}

char ** Parse(char command[])
{
    char **multi_word_commands;

}

int Invoke(char name[], char *args[])
{

}

char ** _Tokenize(const char commands[])
{
    const int length = strlen(commands);
    int counts = 2;
    char **full_command;
    char **command, ***params;
    
}
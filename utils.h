#ifndef __UTILS_H__
#define __UTILS_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define BUFFER_SIZE 1024

#define DEBUG
#ifdef DEBUG
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

#define COMMAND_DISCONNECT "/disconnect"
#define COMMAND_CONNECTED "/connected"
#define COMMAND_LOGOUT "/logout"
#define COMMAND_USERNAME "/username"
#define COMMAND_HELP "/help"

int prefix(const char *str, const char *pre)
{
    return !strncmp(pre, str, strlen(pre));
}

char* remove_prefix(int prefix_length, const char* str)
{
    int str_len = strlen(str);

    if (prefix_length > str_len)
        return NULL;
        
    char* formatted = malloc(str_len - prefix_length + 1);
    strcpy(formatted, &str[prefix_length]);
    return formatted;
}

int is_command(const char* buffer)
{
    return buffer[0] == '/';
}


#endif
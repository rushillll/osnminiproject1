#include <stdlib.h>
#include<stdio.h>
#include <string.h>
#include "pathutil.h"

char* join_path(const char* dir, const char* name)
{
    if (name[0] == '/')
    {
        int len = strlen(name) + 1;
        char* result = malloc(len);
        memcpy(result, name, len);
        return result;
    }

    int len = strlen(dir) + strlen(name) + 2; // '/' + '\0'
    char* result = malloc(len);
    snprintf(result, len, "%s/%s", dir, name);
    return result;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include "locate.h"
#include "pathutil.h"


static int is_executable(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    if (!S_ISREG(st.st_mode)) return 0;
    if (access(path, X_OK) != 0) return 0;
    return 1;
}

static void print_absolute(const char* candidate)
{
    printf("%s\n", candidate);
}

static int locate_one(const char* filename)
{
    int found = 0;

    // 1. Current working directory first.
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        char* candidate = join_path(cwd, filename);
        if (is_executable(candidate))
        {
            print_absolute(candidate);
            found = 1;
        }
        free(candidate);
    }

    const char* path_env = getenv("PATH");
    if (path_env != NULL)
    {
        int len = strlen(path_env) + 1;
        char* path_copy = malloc(len);
        memcpy(path_copy, path_env, len);

        char* dir = strtok(path_copy, ":");
        while (dir != NULL)
        {
            if (dir[0] != '\0')
            {
                char* candidate = join_path(dir, filename);
                if (is_executable(candidate))
                {
                    print_absolute(candidate);
                    found = 1;
                }
                free(candidate);
            }
            dir = strtok(NULL, ":");
        }

        free(path_copy);
    }

    return found;
}

void locate_command(char** args)
{
    if (args == NULL || args[0] == NULL)
    {
        printf("locate: invalid syntax\n");
        return;
    }

    for (int i = 0; args[i] != NULL; i++)
    {
        if (!locate_one(args[i]))
            printf("locate: command not found (%s)\n", args[i]);
    }
}
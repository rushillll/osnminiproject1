#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "executor.h"


static int is_executable(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return 0;
    if (!S_ISREG(st.st_mode))
        return 0;
    if (access(path, X_OK) != 0)
        return 0;
    return 1;
}


static char* resolve_command_path(const char* token)
{

    if (strchr(token, '/') != NULL)
    {
        if (is_executable(token))
        {
            int len = strlen(token) + 1;
            char* path = malloc(len);
            memcpy(path, token, len);
            return path;
        }
        return NULL;
    }

    int skip_cwd = (token[0] == '%');
    const char* name = skip_cwd ? token + 1 : token;

    if (!skip_cwd)
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            char candidate[PATH_MAX + 512];
            snprintf(candidate, sizeof(candidate), "%s/%s", cwd, name);
            if (is_executable(candidate))
            {
                int len = strlen(candidate) + 1;
                char* path = malloc(len);
                memcpy(path, candidate, len);
                return path;
            }
        }
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
                char candidate[PATH_MAX + 512];
                snprintf(candidate, sizeof(candidate), "%s/%s", dir, name);
                if (is_executable(candidate))
                {
                    int clen = strlen(candidate) + 1;
                    char* result = malloc(clen);
                    memcpy(result, candidate, clen);
                    free(path_copy);
                    return result;
                }
            }
            dir = strtok(NULL, ":");
        }

        free(path_copy);
    }

    return NULL;
}

void execute_command(char** argv)
{
    char* path = resolve_command_path(argv[0]);

    if (path == NULL)
    {
        // Error message never includes the '%' prefix, since that's a
        // shell-level instruction, not part of the command's name.
        const char* display = (argv[0][0] == '%') ? argv[0] + 1 : argv[0];
        printf("cshell: command not found (%s)\n", display);
        return;
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        // Child: present argv[0] without the '%' prefix too, so the
        // program sees the name it was actually invoked as.
        if (argv[0][0] == '%')
            argv[0] = argv[0] + 1;

        execv(path, argv);

        // Only reached if execv itself failed (e.g. bad interpreter).
        perror("cshell");
        exit(127);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
    }
    else
    {
        perror("cshell: fork");
    }

    free(path);
}
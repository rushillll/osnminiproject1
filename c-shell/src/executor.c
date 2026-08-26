#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "executor.h"
#include "pathutil.h"


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


char* find_executable(const char* token)
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
            char* candidate = join_path(cwd, name);
            if (is_executable(candidate))
            {
                return candidate;
            }
            free(candidate);
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
                char* candidate = join_path(dir, name);
                if (is_executable(candidate))
                {
                    free(path_copy);
                    return candidate;
                }
                free(candidate);
            }
            dir = strtok(NULL, ":");
        }

        free(path_copy);
    }

    return NULL;
}

void execute_command(char** argv, int input_fd, int output_fd)
{
    char* path = find_executable(argv[0]);

    if (path == NULL)
    {
        const char* display = argv[0];
        if (argv[0][0] == '%') display = argv[0] + 1;

        printf("cshell: command not found (%s)\n", display);

        if (input_fd >= 0) close(input_fd);
        if (output_fd >= 0) close(output_fd);
        return;
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        if (input_fd >= 0)
        {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        if (output_fd >= 0)
        {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        if (argv[0][0] == '%') argv[0] = argv[0] + 1;

        execv(path, argv);

        perror("cshell");
        exit(127);
    }
    else if (pid > 0)
    {
        if (input_fd >= 0) close(input_fd);
        if (output_fd >= 0) close(output_fd);

        int status;
        waitpid(pid, &status, 0);
    }
    else
    {
        perror("cshell: fork");
        if (input_fd >= 0) close(input_fd);
        if (output_fd >= 0) close(output_fd);
    }

    free(path);
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "hop.h"
#include "frecency.h"


static int do_chdir(const char* target, char** previous_cwd)
{
    char before[PATH_MAX];
    if (getcwd(before, sizeof(before)) == NULL) return -1;

    if (chdir(target) != 0) return -1;

    char after[PATH_MAX];
    if (getcwd(after, sizeof(after)) == NULL) return -1;

    free(*previous_cwd);
    int len = strlen(before) + 1;
    *previous_cwd = malloc(len);
    memcpy(*previous_cwd, before, len);

    frecency_record(after);
    return 0;
}

static void hop_one(const char* arg, const char* home, char** previous_cwd)
{
    if (arg == NULL || strcmp(arg, "~") == 0)
    {
        if (do_chdir(home, previous_cwd) != 0) printf("hop: no such directory\n");
        return;
    }

    if (strcmp(arg, ".") == 0) return;

    if (strcmp(arg, "..") == 0)
    {
        if (do_chdir("..", previous_cwd) != 0) printf("hop: no such directory\n");
        return;
    }

    if (strcmp(arg, "-") == 0)
    {
        if (*previous_cwd == NULL) return; 
        if (do_chdir(*previous_cwd, previous_cwd) != 0) printf("hop: no such directory\n");
        return;
    }

    if (do_chdir(arg, previous_cwd) == 0) return;

    char* match = frecency_lookup(arg);
    if (match != NULL)
    {
        do_chdir(match, previous_cwd); 
        free(match);
        return;
    }

    printf("hop: no such directory\n");
}

void hop_command(char** args, const char* home, char** previous_cwd)
{
    if (args == NULL || args[0] == NULL)
    {
        hop_one(NULL, home, previous_cwd);
        return;
    }

    for (int i = 0; args[i] != NULL; i++) hop_one(args[i], home, previous_cwd);
}
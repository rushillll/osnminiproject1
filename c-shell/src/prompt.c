#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "prompt.h"

void display_prompt(char *username, char *hostname, char *homedirectory)
{
    char currentdirectory[PATH_MAX];
    char prompt[PATH_MAX + 256];

    getcwd(currentdirectory, sizeof(currentdirectory)); // current directory

    long long home_directory_length = strlen(homedirectory); // home directory is the one in which the shell was opened

    if (strncmp(currentdirectory, homedirectory, home_directory_length) == 0 &&
        (currentdirectory[home_directory_length] == '/' ||
        currentdirectory[home_directory_length] == '\0'))
    {
        char *remaining = currentdirectory + home_directory_length; // pointer arithmetic, comparing current path to home dir
        snprintf(prompt, sizeof(prompt), "<%s@%s:~%s> ", username, hostname, remaining);
    }
    else
    {
        snprintf(prompt, sizeof(prompt), "<%s@%s:%s> ", username, hostname, currentdirectory);
    }

    printf("%s", prompt);
    fflush(stdout);
}


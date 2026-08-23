#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include "prompt.h"
#include "lexer.h"

int main(void)
{
    char hostname[100];
    char homedirectory[PATH_MAX];

    struct passwd *uid_pw = getpwuid(getuid());
    char *username = uid_pw->pw_name;

    gethostname(hostname, sizeof(hostname));

    getcwd(homedirectory, sizeof(homedirectory)); // directory in which the shell was started

    while (1)
    {
        display_prompt(username, hostname, homedirectory); // user prompt that has the username and hostname x@y
        char input[1025];         
        fgets(input, sizeof(input), stdin);

        Operator* tokens = lexing(input);

        if (lex_error)
        {
            printf("cshell: invalid syntax\n");
            continue;
        }

        if (tokens == NULL) continue;

        free_tokens(tokens);
    }
    return 0;
}
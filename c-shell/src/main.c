#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include "prompt.h"
#include "lexer.h"
#include "parser.h"
#include "hop.h"
#include "reveal.h"
#include "locate.h"


char** extract_argv(Operator* tokens)
{
    int count = 0;
    for (Operator* t = tokens; t != NULL && t->type == Word; t = t->next) count++;

    char** argv = malloc((count + 1) * sizeof(char*));

    int i = 0;

    for (Operator* t = tokens; t != NULL && t->type == Word; t = t->next)
    {
        int len = strlen(t->value) + 1;
        argv[i] = malloc(len);
        memcpy(argv[i], t->value, len);
        i++;
    }
    argv[count] = NULL;
    return argv;
}

void free_argv(char** argv)
{
    for (int i = 0; argv[i] != NULL; i++) free(argv[i]);
    free(argv);
}

int main(void)
{
    char hostname[100];
    char homedirectory[PATH_MAX];
    char* previous_cwd = NULL; // like OLDPWD; NULL until the first hop

    struct passwd *uid_pw = getpwuid(getuid());
    char *username = uid_pw->pw_name;

    gethostname(hostname, sizeof(hostname));

    getcwd(homedirectory, sizeof(homedirectory));

    while (1)
    {
        display_prompt(username, hostname, homedirectory);

        char input[1025];
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        int len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') input[len - 1] = '\0';

        Operator* tokens = lexing(input);

        if (lex_error || !is_valid(tokens))
        {
            printf("cshell: invalid syntax\n");
            free_tokens(tokens);
            continue;
        }

        if (tokens == NULL) continue; 

        char** argv = extract_argv(tokens);
        free_tokens(tokens);

        if (argv[0] != NULL && strcmp(argv[0], "hop") == 0)
        {
            hop_command(argv + 1, homedirectory, &previous_cwd);
        }
        else if (argv[0] != NULL && strcmp(argv[0], "reveal") == 0)
        {
            reveal_command(argv + 1, homedirectory, previous_cwd);
        }
        else if (argv[0] != NULL && strcmp(argv[0], "locate") == 0)
        {
            locate_command(argv + 1);
        }
        
        free_argv(argv);
    }

    free(previous_cwd);
    return 0;
}
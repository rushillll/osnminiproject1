#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <pwd.h>
#include "prompt.h"
#include "lexer.h"
#include "parser.h"
#include "hop.h"
#include "reveal.h"
#include "locate.h"
#include "executor.h"
#include "peek.h"
#include "command.h"
#include "redirect.h"
#include "pipeline.h"


int main(void)
{
    char hostname[100];
    char homedirectory[PATH_MAX];
    char* previous_cwd = NULL;

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

        Pipeline pipeline = extract_pipeline(tokens);
        free_tokens(tokens);

        if (pipeline.count == 1)
        {
            ParsedCommand* command = &pipeline.stages[0];
            char** argv = command->argv;

            int input_fd = -1;
            if (command->infiles[0] != NULL)
            {
                input_fd = open_input_stream(command->infiles);
                if (input_fd < 0)
                {
                    free_pipeline(&pipeline);
                    continue;
                }
            }

            int output_fd = -1;
            pid_t output_sink_pid = -1;
            if (command->outfiles[0].filename != NULL)
            {
                OutputStream output_stream = open_output_stream(command->outfiles);
                if (output_stream.fd < 0)
                {
                    if (input_fd >= 0) close(input_fd);
                    free_pipeline(&pipeline);
                    continue;
                }
                output_fd = output_stream.fd;
                output_sink_pid = output_stream.sink_pid;
            }

            if (argv[0] != NULL && strcmp(argv[0], "hop") == 0)
            {
                int saved_stdin, saved_stdout;
                begin_stdio_redirect(input_fd, output_fd, &saved_stdin, &saved_stdout);
                hop_command(argv + 1, homedirectory, &previous_cwd);
                end_stdio_redirect(saved_stdin, saved_stdout);
            }
            else if (argv[0] != NULL && strcmp(argv[0], "reveal") == 0)
            {
                int saved_stdin, saved_stdout;
                begin_stdio_redirect(input_fd, output_fd, &saved_stdin, &saved_stdout);
                reveal_command(argv + 1, homedirectory, previous_cwd);
                end_stdio_redirect(saved_stdin, saved_stdout);
            }
            else if (argv[0] != NULL && strcmp(argv[0], "locate") == 0)
            {
                int saved_stdin, saved_stdout;
                begin_stdio_redirect(input_fd, output_fd, &saved_stdin, &saved_stdout);
                locate_command(argv + 1);
                end_stdio_redirect(saved_stdin, saved_stdout);
            }
            else if (argv[0] != NULL && strcmp(argv[0], "peek") == 0)
            {
                int saved_stdin, saved_stdout;
                begin_stdio_redirect(input_fd, output_fd, &saved_stdin, &saved_stdout);
                peek_command(argv + 1);
                end_stdio_redirect(saved_stdin, saved_stdout);
            }
            else if (argv[0] != NULL)
            {
                execute_command(argv, input_fd, output_fd);
            }
            else
            {
                if (input_fd >= 0) close(input_fd);
                if (output_fd >= 0) close(output_fd);
            }

            if (output_sink_pid > 0) waitpid(output_sink_pid, NULL, 0);
        }
        else
        {
            run_pipeline(&pipeline, homedirectory, &previous_cwd);
        }

        free_pipeline(&pipeline);
    }

    free(previous_cwd);
    return 0;
}
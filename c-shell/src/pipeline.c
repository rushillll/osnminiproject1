#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "pipeline.h"
#include "redirect.h"
#include "executor.h"
#include "hop.h"
#include "reveal.h"
#include "locate.h"
#include "peek.h"

void run_pipeline(Pipeline* pipeline, const char* home, char** previous_cwd)
{
    int stage_count = pipeline->count;

    int* stage_input_fd = malloc(stage_count * sizeof(int));
    int* stage_output_fd = malloc(stage_count * sizeof(int));
    pid_t* output_sink_pid = malloc(stage_count * sizeof(pid_t));

    for (int i = 0; i < stage_count; i++)
    {
        stage_input_fd[i] = -1;
        stage_output_fd[i] = -1;
        output_sink_pid[i] = -1;
    }

    int setup_failed = 0;

    for (int i = 0; i < stage_count && !setup_failed; i++)
    {
        if (pipeline->stages[i].infiles[0] != NULL)
        {
            stage_input_fd[i] = open_input_stream(pipeline->stages[i].infiles);
            if (stage_input_fd[i] < 0) setup_failed = 1;
        }
    }

    for (int i = 0; i < stage_count && !setup_failed; i++)
    {
        if (pipeline->stages[i].outfiles[0].filename != NULL)
        {
            OutputStream output_stream = open_output_stream(pipeline->stages[i].outfiles);
            if (output_stream.fd < 0)
            {
                setup_failed = 1;
                break;
            }
            stage_output_fd[i] = output_stream.fd;
            output_sink_pid[i] = output_stream.sink_pid;
        }
    }

    if (setup_failed)
    {
        for (int i = 0; i < stage_count; i++)
        {
            if (stage_input_fd[i] >= 0) close(stage_input_fd[i]);
            if (stage_output_fd[i] >= 0) close(stage_output_fd[i]);
            if (output_sink_pid[i] > 0) waitpid(output_sink_pid[i], NULL, 0);
        }
        free(stage_input_fd);
        free(stage_output_fd);
        free(output_sink_pid);
        return;
    }

    int pipe_count = stage_count - 1;
    int (*pipe_fds)[2] = NULL;
    if (pipe_count > 0)
    {
        pipe_fds = malloc(pipe_count * sizeof(int[2]));
        for (int i = 0; i < pipe_count; i++) pipe(pipe_fds[i]);
    }

    pid_t* child_pids = malloc(stage_count * sizeof(pid_t));

    for (int i = 0; i < stage_count; i++)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            if (stage_input_fd[i] >= 0) dup2(stage_input_fd[i], STDIN_FILENO);
            else if (i > 0) dup2(pipe_fds[i - 1][0], STDIN_FILENO);

            if (stage_output_fd[i] >= 0) dup2(stage_output_fd[i], STDOUT_FILENO);
            else if (i < stage_count - 1) dup2(pipe_fds[i][1], STDOUT_FILENO);

            for (int j = 0; j < pipe_count; j++)
            {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            for (int k = 0; k < stage_count; k++)
            {
                if (stage_input_fd[k] >= 0) close(stage_input_fd[k]);
                if (stage_output_fd[k] >= 0) close(stage_output_fd[k]);
            }

            char** argv = pipeline->stages[i].argv;

            // Builtins run directly here, in the child — per spec, a builtin
            // inside a pipeline must not affect the shell's own state (e.g.
            // hop's directory change dies with this process).
            if (argv[0] != NULL && strcmp(argv[0], "hop") == 0)
            {
                hop_command(argv + 1, home, previous_cwd);
                exit(0);
            }
            else if (argv[0] != NULL && strcmp(argv[0], "reveal") == 0)
            {
                reveal_command(argv + 1, home, *previous_cwd);
                exit(0);
            }
            else if (argv[0] != NULL && strcmp(argv[0], "locate") == 0)
            {
                locate_command(argv + 1);
                exit(0);
            }
            else if (argv[0] != NULL && strcmp(argv[0], "peek") == 0)
            {
                peek_command(argv + 1);
                exit(0);
            }

            char* path = find_executable(argv[0]);

            if (path == NULL)
            {
                const char* display = argv[0];
                if (argv[0][0] == '%') display = argv[0] + 1;
                printf("cshell: command not found (%s)\n", display);
                exit(127);
            }

            if (argv[0][0] == '%') argv[0] = argv[0] + 1;

            execv(path, argv);
            perror("cshell");
            exit(127);
        }

        child_pids[i] = pid;
    }

    for (int j = 0; j < pipe_count; j++)
    {
        close(pipe_fds[j][0]);
        close(pipe_fds[j][1]);
    }
    for (int i = 0; i < stage_count; i++)
    {
        if (stage_input_fd[i] >= 0) close(stage_input_fd[i]);
        if (stage_output_fd[i] >= 0) close(stage_output_fd[i]);
    }

    for (int i = 0; i < stage_count; i++)
    {
        int status;
        waitpid(child_pids[i], &status, 0);
    }

    for (int i = 0; i < stage_count; i++)
    {
        if (output_sink_pid[i] > 0) waitpid(output_sink_pid[i], NULL, 0);
    }

    free(pipe_fds);
    free(child_pids);
    free(stage_input_fd);
    free(stage_output_fd);
    free(output_sink_pid);
}
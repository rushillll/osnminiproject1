#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "redirect.h"

#define REDIRECT_CHUNK_SIZE 4096

static int count_infiles(char** infiles)
{
    int count = 0;
    while (infiles[count] != NULL) count++;
    return count;
}

static void feed_files_to_pipe(int* opened_fds, int file_count, int pipe_write_end)
{
    char chunk[REDIRECT_CHUNK_SIZE];

    for (int i = 0; i < file_count; i++)
    {
        ssize_t bytes_read;
        while ((bytes_read = read(opened_fds[i], chunk, sizeof(chunk))) > 0)
        {
            write(pipe_write_end, chunk, bytes_read);
        }
        close(opened_fds[i]);
    }

    close(pipe_write_end);
}

int open_input_stream(char** infiles)
{
    int file_count = count_infiles(infiles);
    int* opened_fds = malloc(file_count * sizeof(int));

    for (int i = 0; i < file_count; i++)
    {
        int fd = open(infiles[i], O_RDONLY);
        if (fd < 0)
        {
            printf("cshell: no such file or directory\n");

            for (int j = 0; j < i; j++) close(opened_fds[j]);
            free(opened_fds);
            return -1;
        }
        opened_fds[i] = fd;
    }

    int pipe_ends[2];
    if (pipe(pipe_ends) != 0)
    {
        for (int i = 0; i < file_count; i++) close(opened_fds[i]);
        free(opened_fds);
        return -1;
    }

    int pipe_read_end = pipe_ends[0];
    int pipe_write_end = pipe_ends[1];

    pid_t feeder_pid = fork();

    if (feeder_pid == 0)
    {
        close(pipe_read_end);
        feed_files_to_pipe(opened_fds, file_count, pipe_write_end);
        free(opened_fds);
        exit(0);
    }

    for (int i = 0; i < file_count; i++) close(opened_fds[i]);
    close(pipe_write_end);
    free(opened_fds);

    return pipe_read_end;
}


static int count_outfiles(OutFileSpec* outfiles)
{
    int count = 0;
    while (outfiles[count].filename != NULL) count++;
    return count;
}

static void fan_pipe_to_files(int pipe_read_end, int* opened_fds, int file_count)
{
    char chunk[REDIRECT_CHUNK_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(pipe_read_end, chunk, sizeof(chunk))) > 0)
    {
        for (int i = 0; i < file_count; i++) write(opened_fds[i], chunk, bytes_read);
    }

    close(pipe_read_end);
    for (int i = 0; i < file_count; i++) close(opened_fds[i]);
}

OutputStream open_output_stream(OutFileSpec* outfiles)
{
    OutputStream result;
    result.fd = -1;

    int file_count = count_outfiles(outfiles);
    int* opened_fds = malloc(file_count * sizeof(int));

    for (int i = 0; i < file_count; i++)
    {
        int flags = O_WRONLY | O_CREAT;
        if (outfiles[i].append) flags |= O_APPEND;
        else flags |= O_TRUNC;

        int fd = open(outfiles[i].filename, flags, 0644);
        if (fd < 0)
        {
            printf("cshell: unable to create file for writing\n");

            for (int j = 0; j < i; j++) close(opened_fds[j]);
            free(opened_fds);
            return result;
        }
        opened_fds[i] = fd;
    }

    int pipe_ends[2];
    if (pipe(pipe_ends) != 0)
    {
        for (int i = 0; i < file_count; i++) close(opened_fds[i]);
        free(opened_fds);
        return result;
    }

    int pipe_read_end = pipe_ends[0];
    int pipe_write_end = pipe_ends[1];

    pid_t sink_pid = fork();

    if (sink_pid == 0)
    {
        close(pipe_write_end);
        fan_pipe_to_files(pipe_read_end, opened_fds, file_count);
        free(opened_fds);
        exit(0);
    }

    close(pipe_read_end);
    for (int i = 0; i < file_count; i++) close(opened_fds[i]);
    free(opened_fds);

    result.fd = pipe_write_end;
    result.sink_pid = sink_pid;
    return result;
}

void begin_stdio_redirect(int input_fd, int output_fd, int* saved_stdin, int* saved_stdout)
{
    *saved_stdin = -1;
    *saved_stdout = -1;

    if (input_fd >= 0)
    {
        *saved_stdin = dup(STDIN_FILENO);
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }
    if (output_fd >= 0)
    {
        *saved_stdout = dup(STDOUT_FILENO);
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }
}

void end_stdio_redirect(int saved_stdin, int saved_stdout)
{
    if (saved_stdin >= 0)
    {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }
    if (saved_stdout >= 0)
    {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
}
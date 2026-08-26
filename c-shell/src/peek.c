#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "peek.h"

#define CHUNK_SIZE 4096

typedef struct
{
    char* data;
    int len;
    int cap;
} LineBuffer;

typedef struct
{
    int enabled;
    int number;
} Numbering;


static void line_buffer_append(LineBuffer* line_buffer, char c)
{
    if (line_buffer->len == line_buffer->cap)
    {
        if (line_buffer->cap == 0) line_buffer->cap = 64;
        else line_buffer->cap = line_buffer->cap * 2;

        line_buffer->data = realloc(line_buffer->data, line_buffer->cap);
    }
    line_buffer->data[line_buffer->len++] = c;
}

/* forward pass over fd, one complete line at a time, no full-file buffering */
static void stream_forward(int fd, void (*on_line)(const char*, int, void*), void* ctx)
{
    LineBuffer line_buffer = {0};
    char chunk[CHUNK_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(fd, chunk, sizeof(chunk))) > 0)
    {
        for (ssize_t i = 0; i < bytes_read; i++)
        {
            if (chunk[i] == '\n')
            {
                on_line(line_buffer.data, line_buffer.len, ctx);
                line_buffer.len = 0;
            }
            else
            {
                line_buffer_append(&line_buffer, chunk[i]);
            }
        }
    }

    if (line_buffer.len > 0) on_line(line_buffer.data, line_buffer.len, ctx);
    free(line_buffer.data);
}

static void count_cb(const char* line, int len, void* ctx)
{
    (void)line;
    if (len > 0) (*(int*)ctx)++;
}

static void print_forward_cb(const char* line, int len, void* ctx)
{
    Numbering* num = ctx;

    if (len == 0)
    {
        printf("\n");
        return;
    }

    if (num->enabled) printf("%d %.*s\n", ++num->number, len, line);
    else printf("%.*s\n", len, line);
}

static int count_nonempty(int fd)
{
    lseek(fd, 0, SEEK_SET);
    int count = 0;
    stream_forward(fd, count_cb, &count);
    return count;
}


static void print_backward_regular(int fd, int n_flag, int* number)
{
    int file_line_count = 0;
    if (n_flag) file_line_count = count_nonempty(fd);

    int current_line_number = *number + file_line_count;

    off_t pos = lseek(fd, 0, SEEK_END);
    if (pos < 0) return;

    if (pos > 0)
    {
        char last_char;
        lseek(fd, pos - 1, SEEK_SET);
        if (read(fd, &last_char, 1) == 1 && last_char == '\n') pos -= 1;
    }

    char* leftover = NULL;
    long leftover_len = 0;
    char chunk[CHUNK_SIZE];

    while (pos > 0)
    {
        long take;
        if (pos > CHUNK_SIZE) take = CHUNK_SIZE;
        else take = pos;

        pos -= take;
        lseek(fd, pos, SEEK_SET);
        read(fd, chunk, take);

        long combined_len = take + leftover_len;
        char* combined = malloc(combined_len);
        memcpy(combined, chunk, take);
        if (leftover) memcpy(combined + take, leftover, leftover_len);
        free(leftover);
        leftover = NULL;

        long segment_end = combined_len;
        for (long i = combined_len - 1; i >= 0; i--)
        {
            if (combined[i] == '\n')
            {
                int line_len = (int)(segment_end - (i + 1));

                if (line_len > 0 && n_flag) printf("%d %.*s\n", current_line_number--, line_len, combined + i + 1);
                else if (line_len > 0) printf("%.*s\n", line_len, combined + i + 1);
                else printf("\n");

                segment_end = i;
            }
        }

        leftover_len = segment_end;
        int leftover_alloc_size = leftover_len;
        if (leftover_alloc_size == 0) leftover_alloc_size = 1;
        leftover = malloc(leftover_alloc_size);
        memcpy(leftover, combined, leftover_len);
        free(combined);
    }

    if (leftover_len > 0)
    {
        if (n_flag) printf("%d %.*s\n", current_line_number--, (int)leftover_len, leftover);
        else printf("%.*s\n", (int)leftover_len, leftover);
    }
    free(leftover);

    *number += file_line_count;
}

/* non-seekable input (stdin/pipe): buffering fully before reversing is allowed */
static void print_reversed_buffered(int fd, int n_flag, int* number)
{
    int data_cap = 4096;
    int data_len = 0;
    char* data = malloc(data_cap);
    char chunk[CHUNK_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(fd, chunk, sizeof(chunk))) > 0)
    {
        while (data_len + bytes_read > data_cap)
        {
            data_cap *= 2;
            data = realloc(data, data_cap);
        }
        memcpy(data + data_len, chunk, bytes_read);
        data_len += bytes_read;
    }

    if (data_len > 0 && data[data_len - 1] == '\n') data_len--;

    int line_cap = 16;
    int line_count = 0;
    int* line_starts = malloc(line_cap * sizeof(int));
    int* line_lens = malloc(line_cap * sizeof(int));

    if (data_len > 0)
    {
        int start = 0;
        for (int i = 0; i <= data_len; i++)
        {
            if (i == data_len || data[i] == '\n')
            {
                if (line_count == line_cap)
                {
                    line_cap *= 2;
                    line_starts = realloc(line_starts, line_cap * sizeof(int));
                    line_lens = realloc(line_lens, line_cap * sizeof(int));
                }
                line_starts[line_count] = start;
                line_lens[line_count] = i - start;
                line_count++;
                start = i + 1;
            }
        }
    }

    int file_line_count = 0;
    if (n_flag)
    {
        for (int i = 0; i < line_count; i++)
            if (line_lens[i] > 0) file_line_count++;
    }

    int current_line_number = *number + file_line_count;
    for (int i = line_count - 1; i >= 0; i--)
    {
        int line_len = line_lens[i];

        if (line_len > 0 && n_flag) printf("%d %.*s\n", current_line_number--, line_len, data + line_starts[i]);
        else if (line_len > 0) printf("%.*s\n", line_len, data + line_starts[i]);
        else printf("\n");
    }

    *number += file_line_count;

    free(line_starts);
    free(line_lens);
    free(data);
}


static int parse_args(char** args, int* n_flag, int* r_flag, char*** files_out, int* file_count_out)
{
    *n_flag = 0;
    *r_flag = 0;

    int cap = 8;
    int count = 0;
    char** files = malloc(cap * sizeof(char*));

    for (int i = 0; args[i] != NULL; i++)
    {
        char* token = args[i];

        if (token[0] == '-' && token[1] != '\0')
        {
            for (int j = 1; token[j] != '\0'; j++)
            {
                if (token[j] == 'n') *n_flag = 1;
                else if (token[j] == 'r') *r_flag = 1;
            }
        }
        else
        {
            if (count == cap)
            {
                cap *= 2;
                files = realloc(files, cap * sizeof(char*));
            }
            files[count++] = token;
        }
    }

    *files_out = files;
    *file_count_out = count;
    return 1;
}

static int check_and_open(const char* filename, int* fd_out, int* is_regular_out)
{
    if (filename == NULL || strcmp(filename, "-") == 0)
    {
        *fd_out = STDIN_FILENO;
        *is_regular_out = 0;
        return 1;
    }

    struct stat st;
    if (stat(filename, &st) != 0)
    {
        printf("peek: no such file or directory\n");
        return 0;
    }
    if (S_ISDIR(st.st_mode))
    {
        printf("peek: is a directory\n");
        return 0;
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        printf("peek: no such file or directory\n");
        return 0;
    }

    *fd_out = fd;
    *is_regular_out = 1;
    return 1;
}

static void peek_one(int fd, int is_regular, int n_flag, int r_flag, int* number)
{
    if (r_flag)
    {
        if (is_regular) print_backward_regular(fd, n_flag, number);
        else print_reversed_buffered(fd, n_flag, number);
    }
    else
    {
        Numbering num = { n_flag, *number };
        stream_forward(fd, print_forward_cb, &num);
        *number = num.number;
    }
}

void peek_command(char** args)
{
    int n_flag, r_flag, file_count;
    char** files;
    if (!parse_args(args, &n_flag, &r_flag, &files, &file_count))
    {
        printf("peek: invalid syntax\n");
        return;
    }

    int number = 0;

    if (file_count == 0)
    {
        int fd, is_regular;
        if (check_and_open(NULL, &fd, &is_regular))
            peek_one(fd, is_regular, n_flag, r_flag, &number);
    }
    else
    {
        for (int i = 0; i < file_count; i++)
        {
            int fd, is_regular;
            if (!check_and_open(files[i], &fd, &is_regular)) continue;

            peek_one(fd, is_regular, n_flag, r_flag, &number);

            if (fd != STDIN_FILENO) close(fd);
        }
    }

    free(files);
}
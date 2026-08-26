#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "reveal.h"
#include "pathutil.h"


static int parse_flag_token(const char* token, int* a_out, int* t_out)
{
    for (int i = 1; token[i] != '\0'; i++)
    {
        if (token[i] == 'a') *a_out = 1;
        else if (token[i] == 't') *t_out = 1;
        else return 0;
    }
    return 1;
}

static int parse_args(char** args, int* a_flag, int* t_flag, char** positional)
{
    *a_flag = 0;
    *t_flag = 0;
    *positional = NULL;

    for (int i = 0; args[i] != NULL; i++)
    {
        char* token = args[i];

        if (token[0] == '-' && token[1] != '\0')
        {
            if (*positional != NULL) return 0; // flags must come before the path
            if (!parse_flag_token(token, a_flag, t_flag)) return 0; // invalid flag character
        }
        else
        {
            if (*positional != NULL) return 0; // more than one positional argument
            *positional = token;
        }
    }

    return 1;
}

static int resolve_target(const char* positional, const char* home, char* previous_cwd, char* out)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, PATH_MAX) == NULL) return 0;

    if (positional == NULL || strcmp(positional, ".") == 0)
    {
        strcpy(out, cwd);
        return 1;
    }

    if (strcmp(positional, "~") == 0)
    {
        strcpy(out, home);
        return 1;
    }

    if (strcmp(positional, "..") == 0)
    {
        char candidate[PATH_MAX + 4];
        snprintf(candidate, sizeof(candidate), "%s/..", cwd);
        if (realpath(candidate, out) == NULL) return 0;
        return 1;
    }

    if (strcmp(positional, "-") == 0)
    {
        if (previous_cwd == NULL) return 0; // no hop has happened yet this session
        strcpy(out, previous_cwd);
        return 1;
    }

    char* candidate = join_path(cwd, positional);
    int ok = (realpath(candidate, out) != NULL);
    free(candidate);
    return ok;
}


static int name_compare(const void* a, const void* b)
{
    const char* sa = *(const char* const*)a;
    const char* sb = *(const char* const*)b;
    return strcmp(sa, sb); // lexicographic, by ASCII value
}


static int list_entries(const char* dir_path, int a_flag, char*** names_out)
{
    DIR* dir = opendir(dir_path);
    if (dir == NULL)
    {
        *names_out = NULL;
        return 0;
    }

    int cap = 8;
    int count = 0;
    char** names = malloc(cap * sizeof(char*));

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (!a_flag && entry->d_name[0] == '.') continue;

        if (count == cap)
        {
            cap *= 2;
            names = realloc(names, cap * sizeof(char*));
        }
        int len = strlen(entry->d_name) + 1;
        names[count] = malloc(len);
        memcpy(names[count], entry->d_name, len);
        count++;
    }

    closedir(dir);
    qsort(names, count, sizeof(char*), name_compare);

    *names_out = names;
    return count;
}

static void free_names(char** names, int count)
{
    for (int i = 0; i < count; i++) free(names[i]);
    free(names);
}


// ls quotes a listed entry in single quotes when its display text contains a space.
static void print_line(const char* text)
{
    if (strchr(text, ' ') != NULL) printf("'%s'\n", text);
    else printf("%s\n", text);
}


static void reveal_recursive(const char* dir_path, const char* prefix, int a_flag)
{
    char** names;
    int count = list_entries(dir_path, a_flag, &names);

    for (int i = 0; i < count; i++)
    {
        char full_path[PATH_MAX + 512];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, names[i]);

        char display[PATH_MAX + 512];
        snprintf(display, sizeof(display), "%s%s", prefix, names[i]);

        struct stat link_st;
        int is_symlink = (lstat(full_path, &link_st) == 0 && S_ISLNK(link_st.st_mode));

        struct stat st;
        if (!is_symlink && stat(full_path, &st) == 0 && S_ISDIR(st.st_mode))
        {
            char display_with_slash[PATH_MAX + 513];
            snprintf(display_with_slash, sizeof(display_with_slash), "%s/", display);
            print_line(display_with_slash);

            char next_prefix[PATH_MAX + 1024];
            snprintf(next_prefix, sizeof(next_prefix), "%s/", display);
            reveal_recursive(full_path, next_prefix, a_flag);
        }
        else
        {
            // Symlinks (even ones pointing at directories) are listed as a
            // single leaf entry; their targets are never recursed into.
            print_line(display);
        }
    }

    free_names(names, count);
}


void reveal_command(char** args, const char* home, char* previous_cwd)
{
    int a_flag, t_flag;
    char* positional;

    if (!parse_args(args, &a_flag, &t_flag, &positional))
    {
        printf("reveal: invalid syntax\n");
        return;
    }

    char target[PATH_MAX];
    if (!resolve_target(positional, home, previous_cwd, target))
    {
        printf("reveal: no such directory\n");
        return;
    }

    struct stat st;
    if (stat(target, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        printf("reveal: no such directory\n");
        return;
    }

    if (t_flag)
    {
        reveal_recursive(target, "", a_flag);
    }
    else
    {
        char** names;
        int count = list_entries(target, a_flag, &names);
        for (int i = 0; i < count; i++)
            print_line(names[i]);
        free_names(names, count);
    }
}
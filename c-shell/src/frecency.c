#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "frecency.h"


#define AGING_THRESHOLD 100.0
#define AGING_FACTOR 0.5

typedef struct
{
    char* path;
    double rank;
    long last_access;
} Entry;


static char* db_file_path(void)
{
    const char* home = getenv("HOME");

    int len = strlen(home) + strlen("/.cshell_hop_db") + 1;
    char* path = malloc(len);
    snprintf(path, len, "%s/.cshell_hop_db", home);
    return path;
}


static int load_entries(Entry** entries_out)
{
    char* path_to_db = db_file_path();
    FILE* f = fopen(path_to_db, "r");
    free(path_to_db);

    if (f == NULL)
    {
        *entries_out = NULL;
        return 0;
    }

    int cap = 8;
    int count = 0;
    Entry* entries = malloc(cap * sizeof(Entry));

    double rank;
    long last_access;
    char path[4096];

    while (fscanf(f, "%lf %ld %4095[^\n]\n", &rank, &last_access, path) == 3)
    {
        if (count == cap)
        {
            cap *= 2;
            entries = realloc(entries, cap * sizeof(Entry));
        }
        entries[count].rank = rank;
        entries[count].last_access = last_access;
        int plen = strlen(path) + 1;
        entries[count].path = malloc(plen);
        memcpy(entries[count].path, path, plen);
        count++;
    }

    fclose(f);
    *entries_out = entries;
    return count;
}

static void save_entries(Entry* entries, int count)
{
    char* path_to_db = db_file_path();
    FILE* f = fopen(path_to_db, "w");
    free(path_to_db);
    if (f == NULL) return;

    for (int i = 0; i < count; i++) fprintf(f, "%f %ld %s\n", entries[i].rank, entries[i].last_access, entries[i].path);
    fclose(f);
}

static void free_entries(Entry* entries, int count)
{
    for (int i = 0; i < count; i++) free(entries[i].path);
    free(entries);
}

void frecency_record(const char* path)
{
    Entry* entries;
    int count = load_entries(&entries);

    int found = -1;
    for (int i = 0; i < count; i++)
    {
        if (strcmp(entries[i].path, path) == 0)
        {
            found = i;
            break;
        }
    }

    long now = (long)time(NULL);

    if (found >= 0)
    {
        entries[found].rank += 1.0;
        entries[found].last_access = now;
    }
    else
    {
        entries = realloc(entries, (count + 1) * sizeof(Entry));
        int plen = strlen(path) + 1;
        entries[count].path = malloc(plen);
        memcpy(entries[count].path, path, plen);
        entries[count].rank = 1.0;
        entries[count].last_access = now;
        count++;
    }

    double total = 0.0;
    for (int i = 0; i < count; i++) total += entries[i].rank;

    if (total > AGING_THRESHOLD)
    {
        for (int i = 0; i < count; i++) entries[i].rank *= AGING_FACTOR;
    }

    save_entries(entries, count);
    free_entries(entries, count);
}


static double recency_weight(long last_access, long now)
{
    double hours_elapsed = (double)(now - last_access) / 3600.0;
    return 1.0 / (1.0 + hours_elapsed);
}

char* frecency_lookup(const char* substr)
{
    Entry* entries;
    int count = load_entries(&entries);

    long now = (long)time(NULL);
    int* tried = calloc(count > 0 ? count : 1, sizeof(int));
    char* result = NULL;


    for (int pass = 0; pass < count && result == NULL; pass++)
    {
        int best = -1;
        double best_score = -1.0;

        for (int i = 0; i < count; i++)
        {
            if (tried[i]) continue;
            if (strstr(entries[i].path, substr) == NULL) continue;

            double score = entries[i].rank * recency_weight(entries[i].last_access, now);
            if (score > best_score)
            {
                best_score = score;
                best = i;
            }
        }

        if (best == -1) break; 

        tried[best] = 1;

        struct stat st;
        if (stat(entries[best].path, &st) == 0 && S_ISDIR(st.st_mode))
        {
            int plen = strlen(entries[best].path) + 1;
            result = malloc(plen);
            memcpy(result, entries[best].path, plen);
        }
    }

    free(tried);
    free_entries(entries, count);
    return result;
}
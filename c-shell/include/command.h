#ifndef COMMAND_H
#define COMMAND_H

#include "lexer.h"

typedef struct
{
    char* filename;
    int append; // 0 = truncate ('>'), 1 = append ('>>')
} OutFileSpec;

typedef struct
{
    char** argv;           // NULL-terminated command words
    char** infiles;        // NULL-terminated input redirection filenames, in order given
    OutFileSpec* outfiles; // terminated by an entry with filename == NULL
} ParsedCommand;

typedef struct
{
    ParsedCommand* stages; // one per pipeline stage, split on '|'
    int count;
} Pipeline;

Pipeline extract_pipeline(Operator* tokens);
void free_pipeline(Pipeline* pipeline);

#endif
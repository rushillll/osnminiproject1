#include <stdlib.h>
#include <string.h>
#include "command.h"

static char* duplicate_string(const char* value)
{
    int len = strlen(value) + 1;
    char* copy = malloc(len);
    memcpy(copy, value, len);
    return copy;
}

// Parses one pipeline stage starting at *token_ptr: its command words and
// its own redirections. Stops at OP_PIPE (and sets *hit_pipe, advancing
// past it so the caller can parse the next stage), or at end of tokens /
// an operator we don't handle yet (OP_SEMI, OP_AMP).
static ParsedCommand extract_stage(Operator** token_ptr, int* hit_pipe)
{
    ParsedCommand stage;
    *hit_pipe = 0;

    int argv_cap = 8;
    int argv_count = 0;
    stage.argv = malloc(argv_cap * sizeof(char*));

    int infiles_cap = 4;
    int infiles_count = 0;
    stage.infiles = malloc(infiles_cap * sizeof(char*));

    int outfiles_cap = 4;
    int outfiles_count = 0;
    stage.outfiles = malloc(outfiles_cap * sizeof(OutFileSpec));

    Operator* token = *token_ptr;

    if (token != NULL && token->type == Word)
    {
        stage.argv[argv_count++] = duplicate_string(token->value);
        token = token->next;
    }

    while (token != NULL)
    {
        if (token->type == Word)
        {
            if (argv_count == argv_cap)
            {
                argv_cap *= 2;
                stage.argv = realloc(stage.argv, argv_cap * sizeof(char*));
            }
            stage.argv[argv_count++] = duplicate_string(token->value);
            token = token->next;
        }
        else if (token->type == Less_Than)
        {
            token = token->next; // grammar guarantees a Word follows OP_LT

            if (infiles_count == infiles_cap)
            {
                infiles_cap *= 2;
                stage.infiles = realloc(stage.infiles, infiles_cap * sizeof(char*));
            }
            stage.infiles[infiles_count++] = duplicate_string(token->value);
            token = token->next;
        }
        else if (token->type == Greater_Than || token->type == GreaterThan_GreaterThan)
        {
            int append_mode = (token->type == GreaterThan_GreaterThan);

            token = token->next; // grammar guarantees a Word follows OP_GT / OP_GTGT

            if (outfiles_count == outfiles_cap)
            {
                outfiles_cap *= 2;
                stage.outfiles = realloc(stage.outfiles, outfiles_cap * sizeof(OutFileSpec));
            }
            stage.outfiles[outfiles_count].filename = duplicate_string(token->value);
            stage.outfiles[outfiles_count].append = append_mode;
            outfiles_count++;
            token = token->next;
        }
        else if (token->type == Pipe)
        {
            *hit_pipe = 1;
            token = token->next; // CMD's WORD starts right after the pipe
            break;
        }
        else
        {
            // Semicolon, Ampersand: not handled yet, stop here.
            break;
        }
    }

    if (argv_count == argv_cap)
    {
        argv_cap += 1;
        stage.argv = realloc(stage.argv, argv_cap * sizeof(char*));
    }
    stage.argv[argv_count] = NULL;

    if (infiles_count == infiles_cap)
    {
        infiles_cap += 1;
        stage.infiles = realloc(stage.infiles, infiles_cap * sizeof(char*));
    }
    stage.infiles[infiles_count] = NULL;

    if (outfiles_count == outfiles_cap)
    {
        outfiles_cap += 1;
        stage.outfiles = realloc(stage.outfiles, outfiles_cap * sizeof(OutFileSpec));
    }
    stage.outfiles[outfiles_count].filename = NULL;
    stage.outfiles[outfiles_count].append = 0;

    *token_ptr = token;
    return stage;
}

Pipeline extract_pipeline(Operator* tokens)
{
    int cap = 4;
    int count = 0;
    ParsedCommand* stages = malloc(cap * sizeof(ParsedCommand));

    Operator* token = tokens;
    int hit_pipe;

    do
    {
        if (count == cap)
        {
            cap *= 2;
            stages = realloc(stages, cap * sizeof(ParsedCommand));
        }
        stages[count++] = extract_stage(&token, &hit_pipe);
    }
    while (hit_pipe);

    Pipeline pipeline;
    pipeline.stages = stages;
    pipeline.count = count;
    return pipeline;
}

void free_pipeline(Pipeline* pipeline)
{
    for (int i = 0; i < pipeline->count; i++)
    {
        ParsedCommand* stage = &pipeline->stages[i];

        for (int j = 0; stage->argv[j] != NULL; j++) free(stage->argv[j]);
        free(stage->argv);

        for (int j = 0; stage->infiles[j] != NULL; j++) free(stage->infiles[j]);
        free(stage->infiles);

        for (int j = 0; stage->outfiles[j].filename != NULL; j++) free(stage->outfiles[j].filename);
        free(stage->outfiles);
    }
    free(pipeline->stages);
}
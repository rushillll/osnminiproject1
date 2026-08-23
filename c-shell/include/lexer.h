#ifndef LEXER_H
#define LEXER_H

typedef enum
{
    Pipe,
    Ampersand,
    Semicolon,
    Less_Than,
    Greater_Than,
    GreaterThan_GreaterThan,
    Word
} OperatorType;

typedef struct Operator
{
    OperatorType type;
    char* value;
    struct Operator* next;
} Operator;


extern int lex_error;

Operator* lexing(char* input);
void free_tokens(Operator* head);

#endif
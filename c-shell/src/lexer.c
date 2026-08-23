#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

int lex_error = 0;

int check_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int check_special(char c)
{
    return c == '|' || c == '&' || c == ';' || c == '<' || c == '>';
}


Operator* generate_token(OperatorType type, const char* value)
{
    Operator* token = malloc(sizeof(Operator));
    token->type = type;

    if (value != NULL)
    {
        int len = strlen(value) + 1; //null terminator
        token->value = malloc(len);
        memcpy(token->value, value, len);
    }
    else
    {
        token->value = NULL;
    }

    token->next = NULL;
    return token;
}
void append(Operator** head, Operator** tail, Operator* token)
{
    if (*head == NULL)
    {
        *head = token;
        *tail = token;
    }
    else
    {
        (*tail)->next = token;
        *tail = token;
    }
}
void free_tokens(Operator* head)
{
    while (head)
    {
        Operator* next = head->next;
        free(head->value);
        free(head);
        head = next;
    }
}

int lex(const char* input, int* i, char* buf)
{
    int b = 0;

    while (input[*i] != '\0' && !check_space(input[*i]) && !check_special(input[*i]))
    {
        char c = input[*i];

        if (c == '\\') //backslash as last character
        {
            char next = input[*i + 1];
            if (next == '\0')
            {
                lex_error = 1;
                return -1;
            }
            buf[b++] = next;
            *i += 2;
        }
        else if (c == '"')
        {
            (*i)++; //consume opening quote
            int closed = 0;
            while (input[*i] != '\0')
            {
                char dc = input[*i];
                if (dc == '"')
                {
                    closed = 1;
                    (*i)++;
                    break;
                }

                if (dc == '\\')
                {
                    char next = input[*i + 1];
                    if (next == '"')
                    {
                        buf[b++] = '"';
                        *i += 2;
                    }
                    else if (next == '\\')
                    {
                        buf[b++] = '\\';
                        *i += 2;
                    }
                    else if (next == '\0')
                    {
                        lex_error = 1;
                        return -1;
                    }
                    else
                    {
                        buf[b++] = '\\';
                        buf[b++] = next;
                        *i += 2;
                    }
                }
                else
                {
                    buf[b++] = dc;
                    (*i)++;
                }
            }

            if (!closed)
            {
                lex_error = 1;
                return -1;
            }
        }
        else if (c == '\'')
        {
            (*i)++; //consume opening quote
            int closed = 0;
            while (input[*i] != '\0')
            {
                if (input[*i] == '\'') //single quotes dont do escape processing
                {
                    closed = 1;
                    (*i)++;
                    break;
                }
                buf[b++] = input[*i];
                (*i)++;
            }

            if (!closed)
            {
                lex_error = 1;
                return -1;
            }
        }
        else
        {
            buf[b++] = c;
            (*i)++;
        }
    }

    buf[b] = '\0';
    return 0;
}

Operator* lexing(char* input)
{
    lex_error = 0;

    if (input == NULL) return NULL;

    Operator* head = NULL;
    Operator* tail = NULL;

    int len = strlen(input);
    char* buf = malloc(len + 1);

    int i = 0;
    while (input[i] != '\0')
    {
        if (check_space(input[i]))
        {
            i++;
            continue;
        }

        char c = input[i];

        if (c == '>')
        {
            if (input[i + 1] == '>') //maximal munch
            {
                append(&head, &tail, generate_token(GreaterThan_GreaterThan, NULL));
                i += 2;
            }
            else
            {
                append(&head, &tail, generate_token(Greater_Than, NULL));
                i += 1;
            }
        }
        else if (c == '<')
        {
            append(&head, &tail, generate_token(Less_Than, NULL));
            i += 1;
        }
        else if (c == '|')
        {
            append(&head, &tail, generate_token(Pipe, NULL));
            i += 1;
        }
        else if (c == '&')
        {
            append(&head, &tail, generate_token(Ampersand, NULL));
            i += 1;
        }
        else if (c == ';')
        {
            append(&head, &tail, generate_token(Semicolon, NULL));
            i += 1;
        }
        else
        {
            if (lex(input, &i, buf) == -1)
            {
                free(buf);
                free_tokens(head);
                return NULL;
            }
            append(&head, &tail, generate_token(Word, buf));
        }
    }
    free(buf);
    return head;
}
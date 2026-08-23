#include <stddef.h>
#include "parser.h"

typedef enum
{
    STATE_NEW_COMMAND,
    STATE_ARG,
    STATE_REDIR_TARGET,
    STATE_BG
} State;

int is_valid(Operator* tokens)
{
    if (tokens == NULL) return 1; //base case

    State state = STATE_NEW_COMMAND;
    Operator* token = tokens;

    while (token != NULL)
    {
        if (state == STATE_NEW_COMMAND)
        {
            if (token->type != Word) return 0;
            state = STATE_ARG;
            token = token->next;
        }
        else if (state == STATE_ARG)
        {
            if (token->type == Word) {} //stay in arg
            else if (token->type == Less_Than || token->type == Greater_Than || token->type == GreaterThan_GreaterThan)
            {
                state = STATE_REDIR_TARGET;
            }
            else if (token->type == Pipe || token->type == Semicolon)
            {
                state = STATE_NEW_COMMAND;
            }
            else // Ampersand
            {
                state = STATE_BG;
            }
            token = token->next;
        }
        else if (state == STATE_REDIR_TARGET)
        {
            if (token->type != Word) return 0;
            state = STATE_ARG;
            token = token->next;
        }
        else // STATE_BG
        {
            if (token->type != Word) return 0;
            state = STATE_ARG;
            token = token->next;
        }
    }
    
    return (state == STATE_ARG || state == STATE_BG);
}
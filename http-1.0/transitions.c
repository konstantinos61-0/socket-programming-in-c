#include "server.h"
/* 
    states: INI, METHOD, URI, VERSION, CR, LF
    Reflect the structure of the grammar divided in tokens,
    not each possible character
*/
// Each ini function takes the current input and the current state, does the transition and any  
// extra output/effect/check needed.

// HTTP tspecials chaaracters
#define TSPECIALS 19
char tspecials[TSPECIALS] = {
    '(', ')', '<', '>', '@', ',', ';', ':', '\\', '\"', '/',
    '[', ']', '\?', '=', '{', '}', ' ', '\t' // SP, HT Included (white space)  
};

// CTLs 
#define CTLS 35
char CTLs[CTLS] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 127
};

void ini_trans(char **current, enum states *state)
{
    if (**current == '\r' || **current == '\n')
        *state = FAILURE;
    else if (**current == ' ')
    {
        *state = INI;
        (*current)++;
    }
    else
        *state = METHOD;
}

void method_trans(char current, enum states *state, int *n, char *method)
{
    if (current == '\r' || current == '\n')
        *state = FAILURE;
    else if (*n <= MAX_METHOD && current == ' ')
    {
        method[*n] = '\0'; 
        if (!strcmp(method, "GET") || !strcmp(method, "HEAD") || !strcmp(method, "POST"))
            *state  = URI;
        else
            *state = FAILURE;
    }
    else if (*n == MAX_METHOD && current != ' ')
        *state = FAILURE;
    else 
    { // *n will never get to be > MAX_METHOD as its incremented by one, and state changes if it reaches MAX_METHOD
        *state = METHOD;
        method[*n] = current;
        (*n)++;
    }
}

void uri_trans(char current, enum states *state, int *n, char *uri)
{
    if (current == '\r' || current == '\n')
        *state = FAILURE;
    else if (*n <= MAX_URI && current == ' ')
    {
        *state = VERSION;
        *(uri + *n) = '\0';
    }
    else if (*n < MAX_URI && current != ' ')
    {
        if (*n == 0 && current != '/')
        {
            *state = FAILURE;
            *(uri) = '\0';
            return;
        }
        *state = URI;
        uri[*n] = current;
        (*n)++;
    }
    else
    {
        *state = FAILURE;
        uri[*n] = '\0';
    }
}

void vers_trans(char current, enum states *state, int *n, char *vers)
{
    if (current == '\n')
        *state = FAILURE;
    else if (*n < VERSION_LEN && current == '\r')
        *state = FAILURE;
    else if (*n < VERSION_LEN && current != '\r')
    {
        *state = VERSION;
        vers[*n] = current;
        (*n)++;
    }
    else if (*n == VERSION_LEN && current == '\r')
    {
        vers[*n] = '\0';
        if (!strcmp(vers, "HTTP/1.0") || !strcmp(vers, "HTTP/1.1"))
            *state = CR; 
        else
            *state = FAILURE;
    }
    else
        *state = FAILURE;
}

void cr_trans(char current, enum states *state)
{
    if (current == '\n')
        *state = LF;
    else
        *state = FAILURE;
}

void lf_trans(char **current, enum states *state, int *n)
{
    if (**current == '\r')
    {
        *state = CR_F;
        (*current)++;
    }
    else if (**current == '\n')
        *state = FAILURE;
    else
    {
        for (int i = 0; i < TSPECIALS; i++) // Skip invalid header field names which contains tspecials or CTL characters 
        {
            if (**current == tspecials[i])
            {
                while (**current != '\r') // On invalid header field name, consume every input until \r, and redo for any header left
                    (*current)++;
                *state = CR;
                (*current)++;
                return;
            }
        }
        // Here current is not a tspecial
        *n = 0; // Initialize the field array index for every new field you get
        *state = HF;
    }
}

void hf_trans(char **current, enum states *state, int *n, int *n_value, header_node *node)
{
    if (*n < MAX_HEADER_FIELD && **current != ':')
    {
        for (int i = 0; i < TSPECIALS; i++) // Skip invalid header field names which contains tspecials or CTL characters 
        {
            if (**current == tspecials[i])
            {
                while (**current != '\r')
                    (*current)++;
                *state = CR;
                return; // Early return when Reseting state back to CR after invalid header field name
            }
        }
        node->header_field[*n] = **current;
        (*n)++;
        *state = HF;
    }
    else if (*n <= MAX_HEADER_FIELD && **current == ':')
    {
        node->header_field[*n] = '\0';
        *n_value = 0; // Initialize the field value array index for every new field you get
        *state = HVAL;
    }
    else // Longer than allowed header field names
    {
        if (**current == '\r')
            *state = CR;
        else
            *state = FAILURE;
    }
}


void hval_trans(char current, enum states *state, int *n, header_node *node, header_node **list)
{
    if (current == '\n')
        *state = FAILURE;
    else if (*n < MAX_HEADER_VALUE && current == ' ')
        *state = HVAL;
    else if (*n < MAX_HEADER_VALUE && current != '\r')
    {
        *state = HVAL;
        node->header_value[*n] = current;
        (*n)++;
    }
    else if (*n <= MAX_HEADER_VALUE && current == '\r')
    {
        *state = CR;
        node->header_value[*n] = '\0';
        push_node(list  , node);
    }
    else // Longer than allowed header value
    {
        if (current != '\r')
            *state = HVAL;
        else
            *state = LF; 
    }
}

void cr_f_trans(char current, enum states *state)
{
    if (current == '\n')
        *state = LF_F;
    else
        *state = FAILURE;
}
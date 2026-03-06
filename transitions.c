#include "server.h"
/* 
    The states reflect the structure of the HTTP grammar divided in tokens

    Each "foo_trans" function defined in this file takes the current input and state, does the transition and any  
    extra output/effect/check needed.

    Whitespace: space or horizontal tab characters
    In general, the states accept any number of leading whitespace before parsing their according structure

    When a request structure (e.g. method, uri or header field name) exceeds my servers' char array limits, a 400 response is set.
    On invalid headers, a 400 response is set.
*/

// Test functions prototypes
int test_method(char *method);
int test_uri(char *uri, char *filename);


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

void method_trans(char current, enum states *state, int *n, char *method)
{
    if (current == '\r' || current == '\n')
        *state = FAILURE_400;
    else if (*n == 0 && (current == ' ' || current == '\t'))
        *state = METHOD; 
    else if (*n <= MAX_METHOD && (current == ' ' || current == '\t'))
    {
        method[*n] = '\0'; 
        // test method
        int test = test_method(method);
        if (test == 501)
            *state = FAILURE_501_METHOD;
        else if (test == 400)
            *state = FAILURE_400_METHOD;
        else
            *state = URI;
    }
    else if (*n == MAX_METHOD && (current != ' ' && current != '\t'))
        *state = FAILURE_400_METHOD;
    else 
    { // *n will never get to be > MAX_METHOD as its incremented by one, and state changes if it reaches MAX_METHOD
        *state = METHOD;
        method[*n] = current;
        (*n)++;
    }
}

void uri_trans(char current, enum states *state, int *n, char *uri, char *filename)
{
    if (current == '\r' || current == '\n')
        *state = FAILURE_400;
    else if (*n == 0 && (current == ' ' || current == '\t'))
        *state = URI;
    else if (*n <= MAX_URI && (current == ' ' || current == '\t'))
    {
        *(uri + *n) = '\0';
        int test = test_uri(uri, filename);
        if (test == 500)
            *state = FAILURE_500;
        else if (test == 400)
            *state = FAILURE_400;
        else
            *state = VERSION;
    }
    else if (*n < MAX_URI && (current != ' ' && current != '\t'))
    {
        if (*n == 0 && current != '/')
        {
            *state = FAILURE_400;
            *(uri) = '\0';
            return;
        }
        *state = URI;
        uri[*n] = current;
        (*n)++;
    }
    else
        *state = FAILURE_400;
}

void vers_trans(char current, enum states *state, int *n, char *vers)
{
    if (current == '\n')
        *state = FAILURE_400;
    else if (*n == 0 && (current == ' ' || current == '\t'))
        *state = VERSION;
    else if (*n < VERSION_LEN && current == '\r')
        *state = FAILURE_400;
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
            *state = FAILURE_400;
    }
    else
        *state = FAILURE_400;
}

void cr_trans(char current, enum states *state)
{
    if (current == '\n')
        *state = LF;
    else
        *state = FAILURE_400;
}

void lf_trans(char **current, enum states *state, int *n)
{
    if (**current == '\r')
    {
        *state = CR_F;
        (*current)++;
    }
    else if (**current == '\n')
        *state = FAILURE_400;
    else
    {
        for (int i = 0; i < CTLS; i++) 
        {
            if (**current == CTLs[i] || (i < TSPECIALS && **current == tspecials[i]))
            {
                *state = FAILURE_400; // On invalid header field name, respond 400
                return;
            }
        }
        // Here current is a valid header field name character
        *n = 0; // Initialize the field array index for every new field you get
        *state = HF;
    }
}

void hf_trans(char current, enum states *state, int *n, int *n_value, header_node *node)
{
    if (*n < MAX_HEADER_FIELD && current != ':')
    {
        for (int i = 0; i < CTLS; i++) 
        {
            if (current == CTLs[i] || (i < TSPECIALS && current == tspecials[i]))
            {
                *state = FAILURE_400; // On invalid header field name, respond 400
                free(node);
                return;
            }
        }
        node->header_field[*n] = current;
        (*n)++;
        *state = HF;
    }
    else if (*n <= MAX_HEADER_FIELD && current == ':')
    {
        node->header_field[*n] = '\0';
        *n_value = 0; // Initialize the field value array index for every new field you get at HF -> HVAL
        *state = HVAL;
    }
    else // Longer than allowed header field names
    {
        free(node);
        *state = FAILURE_400;
    }
        
}


void hval_trans(char current, enum states *state, int *n, header_node *node, header_node **list)
{
    if (current == '\n')
    {
        *state = FAILURE_400;
        free(node);
    }
    else if (*n == 0 && (current == ' ' || current == '\t'))
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
        push_node(list, node);
    }
    else // Longer than allowed header value
    {
        *state = FAILURE_400;
        free(node);
    }
}

void cr_f_trans(char current, enum states *state)
{
    if (current == '\n')
        *state = LF_F;
    else
        *state = FAILURE_400;
}

// Below i define some test funtions used inside the transitions

// Tests the validity of parsed method and returns 0 at success or the corresponging code at failure.
int test_method(char *method)
{
    if (!strcmp(method , "POST") || !strcmp(method , "HEAD"))
        return 501;
    else if (strcmp(method, "GET"))
        return 400;
    else
        return 0;
}


// Tests the validity of parsed URI and returns 0 at success or the corresponging code at failure.
// Also extracts the filename from the uri
int test_uri(char *uri, char *filename)
{

    if (!strcmp(uri, "/")) // uri = /
        strcpy(filename, "index.html");
    else if (uri[strlen(uri) - 1] == '/') // uri ends in /
    {
        int tmp_len = strlen(uri) + strlen("index.html"), size;
        char tmp[tmp_len];
        if (tmp_len > MAX_URI) // tmp len includes the null termination byte in its calculation
            return 400;
        else if ((size = snprintf(tmp, tmp_len,"%s%s", (uri + 1), "index.html")) < 0 || size >= tmp_len)
            return 500;
        else 
            strcpy(filename, tmp);
    } // uri doesnt end in /
    else
        strcpy(filename, (uri + 1)); 
    return 0;   
}
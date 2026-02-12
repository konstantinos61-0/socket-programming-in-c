#include "my_header.h"


/*
    This function implements the Parser (FSM) and
    runs once for each connection.
*/
void handle_connection(int client_sockfd, int root_dir) 
{   
    char req_buf[BUFF_SIZE];
    char *res_buf = malloc(BUFF_SIZE);
    int req_len = BUFF_SIZE, res_len = BUFF_SIZE;
    if (read_request(client_sockfd, req_buf, &req_len) == -1)
    {
        return ;
    }
    enum {IN, METHOD, U, END} state;
    state = IN;
    int ok = 0;
    char *current = NULL;
    while (state != END)
    {
        char *word;
        word = read_input(req_buf, &current);
        switch ( state )
        {
            case IN:
                if (!strcmp(word, "GET"))
                    state = METHOD;
                else
                    state = END;
                break;
            case METHOD:
                if (word[0] != '/')
                {
                    state = END;
                    break;
                }
                ok = 1;
                char *filename = word + 1;
                if (strlen(filename) == 0)
                    serve("index.html", root_dir, res_buf, &res_len);
                else
                    serve(filename, root_dir, res_buf, &res_len);
                state = U;
                break;
            case U:
                if (strcmp(word, "\r\n"))
                {
                    state = END;
                }
                else
                    state = END;
                // buffer the contents of the found file 
                break;
        }
        printf("word: %s\n", word);
        free(word);
    }
    if (ok)
    {
        send_all(client_sockfd, res_buf, &res_len);
        free(res_buf);
        return;
    }
}
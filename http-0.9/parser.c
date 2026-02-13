#include "my_header.h"


/*
    This function implements the Parser (FSM) and
    runs once for each connection.
*/
void handle_connection(int client_sockfd, int root_dir) 
{   
    char req_buf[BUFF_SIZE];
    int req_len = BUFF_SIZE;
    char *res_buf = malloc(BUFF_SIZE);
    int res_len = BUFF_SIZE;

    char *filename = malloc(sizeof(char));
    enum {IN, METHOD, U, END} state;
    state = IN;
    char *current = NULL;

    if (res_buf == NULL)
    {
        free(res_buf);
        return;
    }
    if (read_request(client_sockfd, req_buf, &req_len) == -1)
    {
        free(res_buf);
        return ;
    }
    while (state != END)
    {
        char *word;
        word = read_input(req_buf, &current);
        if (word == NULL)
        {
            free(word);
            free(res_buf);
            return;
        }
        switch (state)
        {
            case IN:
                if (!strcmp(word, "GET"))
                    state = METHOD;
                else
                {
                    serve("error.html", root_dir, res_buf, &res_len);
                    state = END;
                }
                break;
            case METHOD:
                if (!strcmp(word, "\r\n"))
                {
                    serve("error.html", root_dir, res_buf, &res_len);
                    state = END;
                }
                else if (word[0] != '/')
                {
                    serve("error.html", root_dir, res_buf, &res_len);
                    state = END;
                }
                else if (strlen(word) == 1)
                {
                    filename = realloc(filename, strlen("index.html") + 1);
                    strcpy(filename, "index.html");
                    state = U;
                }
                else
                {
                    filename = realloc(filename, strlen(word) + 1);
                    strcpy(filename, word + 1);
                    state = U;
                }
                break;
            case U:
                if (!strcmp(word, "\r\n"))
                {
                    if (serve(filename, root_dir, res_buf, &res_len) == -1)
                        serve("not_found.html", root_dir, res_buf, &res_len);
                    state = END;
                }
                else
                {
                    serve("error.html", root_dir, res_buf, &res_len);
                    state = END;
                }
                break;
        }
        free(word);
    }
    if (send_all(client_sockfd, res_buf, &res_len) == -1)
    {
        perror("server send");
    }
    free(res_buf);
}
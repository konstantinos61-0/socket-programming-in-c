#include "server.h"

// ready-to-use response line struct variables
struct response_line c200 = {
    200, "OK", NULL
};

struct response_line c400 = {
    400, "Bad Request", "Error 400: Bad Request"
};

struct response_line c404 = {
    404, "Not Found", "Error 404: File Not Found"
};

struct response_line c403 = {
    403, "Forbidden", "Error 403: Forbidden"
};

struct response_line c500 = {
    500, "Internal Server Error", "500: Internal Server Error"
};

struct response_line c501 = {
    501, "Not Implemented", "501: Not Implemented"
};

/*
    This file contains only this function.
    It implements the Parser (according to the Finite state transducer abstract software model specified in the README) 
    and sends the back a properresponse.
    It runs once for each connection.
    On socket send/recv error, a clean up is performed and the connection is closed.
*/
void handle_connection(int client_sockfd, int root_dir) 
{   
    // Static file info
    int filefd;
    char filename_original[MAX_URI]; // uri max value -1 for omitted "/". Will be extracted from uri and won't change
    char filename_final[MAX_URI]; // Will be the final file to be sent (error template or original file depending on response code)
    char uri[MAX_URI + 1];

    // buffer info
    char res_buf[BUFF_SIZE];
    char *req_buf = malloc(BUFF_SIZE * sizeof(char));
    int req_len = BUFF_SIZE * sizeof(char), res_len = BUFF_SIZE * sizeof(char);
    int m;

    // parsing info 
    int n_method = 0, n_vers = 0, n_uri = 0, n_field = 0, n_value = 0;
    char method[MAX_METHOD + 1], vers[VERSION_LEN + 1];
    struct response_line results;
    enum states state = METHOD, old_state;
    header_node *req_h_list = NULL, *res_h_list = NULL, *n;
    memset(&results, '\0', sizeof results);

    if (req_buf == NULL)
    {
        results = c500;
        state = FAILURE; // Causes a 500 response before any request parsing
    }

    // Only on successful req_buf mem allocation, continue to read the request.
    if (req_buf != NULL && (m = read_request(client_sockfd, &req_buf, &req_len)) == -1)// recv error 
    {
        results = c500;
        state = FAILURE; 
    }
    else if (m == -2) // Client closed connection prematurely
    {
        free(req_buf);
        close(client_sockfd);
        return;
    } 
    else if (m == -3)
    {
        results = c400;
        results.msg = "Request exceeded size limit";
        state = FAILURE;
    }
    // Character Input, one at a time
    char *current = req_buf;

    while (current < (req_buf + req_len) && state != FAILURE && state != LF_F) // Request-Line Parser Loop
    {
        //  In each current input i call the associated transition function, defined in transitions.c
        //  which performs any output + the actual state transition δ.
        old_state = state;
        switch (state)
        {
            case METHOD:
                method_trans(*current, &state, &n_method, method);
                break;
            case URI:
                uri_trans(*current, &state, &n_uri, uri);
                break;
            case VERSION:
                vers_trans(*current, &state, &n_vers, vers);
                break;
            case CR:
                cr_trans(*current, &state);
                break;
            case LF:
                lf_trans(&current, &state, &n_field);
                    if (state == HF && old_state == LF)
                        n = malloc(sizeof(header_node));
                continue; // The current input is incremented (or not, depending on input) inside the lf_trans 
            case HF:
                hf_trans(*current, &state, &n_field, &n_value, n); 
                break; 
            case HVAL:
                hval_trans(*current, &state, &n_value, n, &req_h_list);
                break;
            case CR_F:
                cr_f_trans(*current, &state);
                break;
        }
        current++; 
    } // End Request-Line Parser Loop
    
    free(req_buf);
    // Set results structure to the correct corresponding response code (cxxx e.g. c200 for 200 OK)

    if (state == LF_F)
        results = c200;
    else
        results = c400;

    // Extract filename from URI. Filename always corresponds to the file referenced by the uri. Doesn't change
    if (!strcmp(uri, "/")) // uri = /
        strcpy(filename_original, "index.html");
    else if (uri[strlen(uri) - 1] == '/') // uri ends in /
    {
        char tmp[strlen(uri) + strlen("index.html")];
        sprintf(tmp, "%s%s", (uri + 1), "index.html");
        strcpy(filename_original, tmp);
    } // uri doesnt end in /
    else
        strcpy(filename_original, (uri + 1));

    // Check for forbidden pathname patterns and modify results accordingly
    if (strstr(uri, "..") != NULL)
    {
        results = c403;
        results.msg = "Pattern .. not allowed pattern inside a pathname";
    }

    // Try to open the Requested resource (static file) and modify results accordingly
    if (results.code != 403 && results.code != 400)
    {
        if ((filefd = openat(root_dir, filename_original, O_RDONLY)) == -1 && errno == ENOENT)
            results = c404;
        else if (filefd == -1 && errno == EACCES)
        {
            results = c403;
            results.msg = "Access to the file was denied due to insufficient permissions";
        }
        else if (filefd == -1)
            results = c500;
    }
    // Check if the filefd opened a directory, and extract its index.html as the filename_original.
    struct stat sb;
    if (filefd != -1 && fstat(filefd, &sb) != -1)
    {
        if ((sb.st_mode & S_IFMT) == S_IFDIR)
        {
            char tmp_2[strlen(filename_original) + strlen("index.html") + 2];
            sprintf(tmp_2, "%s/%s", filename_original, "index.html");
            strcpy(filename_original, tmp_2);
            filefd = openat(root_dir, filename_original, O_RDONLY);
        }
    }

    // Check  for not implemented method and modify results accordingly
    if (results.code == 200)
    {   
        if (!strcmp(method , "POST") || !strcmp(method , "HEAD"))
        {
            results = c501;
            results.msg = "The applied method is not supported by the server";
        }
    }
    // results is now set to the correct corresponding code value cxxx.

    // Open the correct error template if there is an error code
    if (results.code == 500)
    {
        strcpy(filename_final, "templates/internal_error.html");
        filefd = openat(root_dir, "templates/internal_error.html", O_RDONLY);
    }     
    else if (results.code != 200)
    {
        strcpy(filename_final, "templates/error.html");
        filefd = openat(root_dir, "templates/error.html", O_RDONLY);
    }
    else 
        strcpy(filename_final, filename_original);
    
    // filefd exists either as error html template or as the valid filename to be served.
    // filename_final is the name of the file to be served. Its only different than the original
    // in case of an error status code.

    
    

    // Send response 
    
    // Send response line
    char response_line[RESPONSE_LINE_LEN + 1];
    int bytes_read;
    sprintf(response_line, "HTTP/1.0 %i %s\r\n", results.code, results.phrase);
    int res_line_len = strlen(response_line);
    if (send_all(client_sockfd, response_line, &res_line_len) == -1)
    {
        perror("server send");
        close(client_sockfd);
        close(filefd);
        return;
    }

    // Send response file / error template 
    if (results.code != 200 && results.code != 500)
    {
        if (serve_error_template(client_sockfd, filefd, results.msg, filename_final) != -1) // Serves the headers as well
        {
            // Diagnostic info printed to server's terminal
            printf("\033[31m%s %s %i %s\n\033[0m", method, filename_original, results.code, results.phrase);
            close(filefd);
            return;
        }
        else
        {
            close(filefd);
            strcpy(filename_final, "templates/internal_error.html");
            results = c500;
            filefd = openat(root_dir, filename_final, O_RDONLY);
        }
    }

    // Send response headers
    fill_response_headers(filefd, filename_final, &res_h_list, NULL); // Fill and send response headers 
    // Loop into linked list and send each
    for (header_node *p = res_h_list; p != NULL; p = p->next)
    {
        char resp_header[MAX_HEADER_FIELD + MAX_HEADER_VALUE + 5]; // +4 for the 4 extra chars from field, value. +1 for '\0'
        sprintf(resp_header,"%s: %s\r\n", p->header_field, p->header_value);
        int rhlen = strlen(resp_header);
        if (send_all(client_sockfd, resp_header, &rhlen) == -1)
        { 
            perror("server send");
            close(client_sockfd);
            close(filefd);
            free_list(req_h_list);
            free_list(res_h_list);
            return;
        }
    }
    char *crlf = "\r\n";
    int crlf_len = strlen(crlf);
    if (send_all(client_sockfd, crlf, &crlf_len) == -1)
    {
        perror("server send");
        close(client_sockfd);
        close(filefd);
        return;
    }

    // Send file contents
    while ((bytes_read = read(filefd, res_buf, res_len)) > 0)
    {
        if (send_all(client_sockfd, res_buf, &bytes_read) == -1)
        {
            close(client_sockfd);
            close(filefd);
            perror("server send");
            return;
        } 
    }
    if (bytes_read == -1)
    {
        close(client_sockfd);
        close(filefd);
        perror("server read");
        return;
    }
    
    // Diagnostic info printed to server's terminal
    if (results.code == 200)
        printf("\033[32m%s %s %i %s\n\033[0m", method, filename_original, results.code, results.phrase);
    else
        printf("\033[31m%s %s %i %s\n\033[0m", method, filename_original, results.code, results.phrase);

    close(filefd);
    free_list(req_h_list);
    free_list(res_h_list);
    return;
}


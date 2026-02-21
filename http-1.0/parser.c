#include "server.h"
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
    Notes:
    1. Didn't include error checking 500 answer to send_all, openat for templates.
    Prothikes
    0. Request Buffer Dynamic Size !! DONE
    1. Error checking DONE
    2. Variable organizing DONE
    3. Header parsing TODO
    4. Header buffering TODOssssssss
    5. Response line struct usage DONE
    6. Template generation!! + Fix the templates' html to be modern. TOFIX 
*/ 
/*
    This function implements the Parser (FSM) + Sends the response back
    It runs once for each connection.
*/
void handle_connection(int client_sockfd, int root_dir, int trans[STATES - 2][INPUTS]) 
{   
    // Static file info
    int filename_len = 0;
    int filefd;
    char filename[FILENAME_LEN];

    // buffer info
    char res_buf[BUFF_SIZE];
    char *req_buf = malloc(BUFF_SIZE * sizeof(char));
    int req_len = BUFF_SIZE * sizeof(char);
    int res_len = BUFF_SIZE * sizeof(char);
    int m;

    // parsing info 
    char *current = req_buf;
    char *method = NULL;
    struct response_line results;
    enum states state = M;
    enum states new_state = M;
    header_node *h_list = NULL;
    memset(&results, '\0', sizeof results);

    if (req_buf == NULL)
        results = c500;

    
    if (req_buf != NULL && (m = read_request(client_sockfd, &req_buf, &req_len)) == -1)// recv error 
    {
        results = c500;
        new_state = FAILURE;
    }
    else if (m == -2) // Client closed connection prematurely
        return; 

    while (new_state != SUCCESS && new_state != FAILURE) // Request-Line Parser Loop
    {
        new_state = trans[state][*current];
        // Save method
        if (new_state == URI && state == M_T)
            method = "GET";
        else if (new_state == URI && state == M_T2)
            method = "POST";
        else if (new_state == URI && state == M_D)
            method = "HEAD";

        // Save filename
        if (new_state == URI_CHAR && state == URI_CHAR)
        {
            filename[filename_len] = *current;
            filename_len++;
        }

        // Fill results struct at FAILURE / SUCCESS
        if (new_state == FAILURE)
            results = c400;
        else if (new_state == SUCCESS && !strcmp(method, "GET"))
            results = c200;
        else if (new_state == SUCCESS && strcmp(method, "\0"))
        {
            results = c501;
            results.msg = "The applied method is not supported by the server";
        }
        current++; // Since request line always ends on CRLF, my transition table + loop conditions enforce that current
                   //  wont ever incremenet past the request buffer's loaded data.
        state = new_state; 
    } // End Request-Line Parser Loop
    filename[filename_len] = '\0';

    // TODO: Parse and save headers in a linked list
    /*
        1. Parse request buffer
        2. Save header in a header_node struct
        3. Push the header item into the list
    */
    /*
    struct header_node h;
    while(memmem(current, req_len - current, "\r\n", 2) != current && *current != ':') // While current doesnt point exactly "/r/n" it means there are headers to parse
    {
        // Parse and save headers beginning in current. use push_header function.
        if (*current != ':')
        
        current++;
    }
    */

    free(req_buf);

    // Handle "/" URI
    if (!strcmp(filename, ""))  
        strcpy(filename, "index.html");

    // Check for forbidden patterns like "/../" inside filename
    if (results.code == 200 && strstr(filename, "..") != NULL)
    {
        results = c403;
        results.msg = "Pattern /../ not allowed pattern inside a pathname";
    }
    // Try to open the Requested resource (static file) 
    else if (results.code == 200)
    {
        if ((filefd = openat(root_dir, filename, O_RDONLY)) == -1 && errno == ENOENT)
            results = c404;
        else if (filefd == -1 && errno == EACCES)
        {
            results = c403;
            results.msg = "Access to the file was denied due to insufficient permissions";
        }
        else if (filefd == -1)
            results = c500;
    }
    // Open the 
    if (results.code != 200 && results.code != 500)
    {
        filefd = openat(root_dir, "templates/error.html", O_RDONLY);
    }
    else if (results.code == 500)
    {
        filefd = openat(root_dir, "templates/internal_error.html", O_RDONLY);
    }
    // From here on out, filefd exists either as error.html or the correct filename to be served.

    // Send response 

    char response_line[RESPONSE_LINE_LEN], type[HEADER_VALUE_LEN];
    int bytes_read;
    char *crlf = "\r\n";
    int crlf_len = strlen(crlf);

    sprintf(response_line, "HTTP/1.0 %i %s%s", results.code, results.phrase, crlf);

    /*
        TODO: Replace with systematic header handling!! (define a function to format the Content-Type header)
        Hint: create a function to read the file extension and return the correct MIME Type to pass into the header value.
    */
    sprintf(type, "Content-Type: %s%s", mime_type(filename), crlf);

    int res_line_len = strlen(response_line);
    int type_len = strlen(type);

    send_all(client_sockfd, response_line, &res_line_len);
    send_all(client_sockfd, type, &type_len);
    send_all(client_sockfd, crlf, &crlf_len);




    // Send response file / error template 
    if (results.code != 200 && results.code != 500)
    {
        if (serve_error_template(client_sockfd, filefd, results.msg) != -1)
        {
            // Diagnostic info printed to server's terminal
            if (results.code == 400)
                printf("\033[31m%i %s\n\033[0m", results.code, results.phrase);
            else
                printf("\033[31m%s %i %s %s\n\033[0m", filename, results.code, method, results.phrase);
            close(filefd);
            return;
        }
        else
        {
            close(filefd);
            strcpy(filename, "templates/internal_error.html");
            results = c500;
            filefd = openat(root_dir, filename, O_RDONLY);
        }
    }
    while ((bytes_read = read(filefd, res_buf, res_len)) > 0)
    {
       send_all(client_sockfd, res_buf, &bytes_read); 
    }
    
    // Diagnostic info printed to server's terminal
    if (results.code == 200)
        printf("\033[32mserved %s\n\033[0m", filename);
    else if (results.code == 400)
        printf("\033[31m%i %s\n\033[0m", results.code, results.phrase);
    else
        printf("\033[31m%s %i %s %s\n\033[0m", filename, results.code, method, results.phrase);

    close(filefd);
    return;
}



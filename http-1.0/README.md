# HTTP/1.0 compliant server program

### Description

This project is a CLI program written in C which can be build with the "make" command according to the makefile included in the project. Default executable name is "server". This server implements a portion of the HTTP/1.0 application layer protocol over TCP connections in the Internet Domain of communication (IP Addresses). Specifically, its functionality is focused on serving static files hosted by the host computer.

**Program Usage**: server root_dir [port].  
- **server**: the default executable file name
- **root_dir**: The directory that the server will use as its root for serving files. For example, the URL "/my_page.html" will correspond to the file stored on the server at "root_dir/my_page.html"
- **port**: The port number that the server will bind to on the localhost. If ommited, 8080 will be used by default.

I divided it into multiple source files so that each one has a clear functionality / purpose; server.c contains the main function, parser.c contains the HTTP request parser and helpers.c contains all the smaller utility functions that i wrote for detail abstraction and readability improvement over the project's source files. Since this is a medium-sized project i centralized all the shared definitions/declarations, placing them into a single header file, server.h, for easier maintenance.

### HTTP/1.0 request parser 

Modelled as a Deterministic Finite State Machine of the transducer type:

#### Specification

1) The parser handles one HTTP/1.0 request/connection, per the RFC specification.
2) The parser responds to valid HTTP/1.0 GET requests, returning the appropriate entity-body identified by the Request URL.
3) The parser responds to any valid non-GET HTTP/1.0 request with a 501 status code response
4) The parser responds with a 500 status code response when encountering an unexpected condition preventing it to fullfil the request (any fatal program errors)
5) The parser responds with a 400 response to syntactically malformed requests
6) The parser responds with 403 when no permissions to access the resource are given on the host device.
7) The parser responds with 404 to valid HTTP/1.0 GET requests that have a URI not matching any server hosted resource.
8) The parser will store all header field name/value pairs in a request
9) The parser always includes the Content-Type header field in the response.

#### Design:
Inputs:

- The input alphabet Σ is the ASCII charset symbols/bytes
- The complete input string that will be run through the machine is taken from a character array(buffer) which is prefilled with the request bytes.
- word: For each state transition, input is consumed in the form of a word, which is just a string over Σ.
In the HTTP context,(assuming canonical request form for simplicity) words in the buffer are seperated by space.
- CRLF is the exception to the input word consumption rule since the parser considers it a new word without it being seperated with a space from previous word.

Outputs:

- The output alphabet is any raw data byte (ASCII or not)
- store x: Store output string x as a program variable 
- socket x: Send output string x over the TCP connection to the client.

States:

- Initial: No input has been yet consumed
- Method: Method has just been read
- URI: Request-URI has just been read
- End: When the machine Reaches this state it has completed its functionality and thus its operation is terminated.

Included response headers:
Connection: close (don't support persistent connections)



Used Valgrind to make it run memcheck clean
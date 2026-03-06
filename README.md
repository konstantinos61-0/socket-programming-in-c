# HTTP/1.0 server program (partial implementation)

## Description

This project is a CLI program written in C. It is an http server program with its functionality focused on serving static files hosted by the host computer on some specified "root" directory (although it is designed with extensibility in mind). More specifically, is an implementation of a portion of the HTTP/1.0 application layer protocol over TCP connections in the Internet Domain (IP Addresses). The choice of  the older -though substantial- version 1.0 was deliberate as i wanted to balance  my project's scope between being it being a valuable learning experience for me while remaining relevant to real world work.  

A makefile is included for the build. Running "make" produces an executable named "server".

**Program Usage**: server root_dir [port].  
- **server**: the default executable file name by make
- **root_dir**: The directory that the server will use as its root for serving files. For example, the URL "/my_page.html" will correspond to the file stored on the server at "root_dir/my_page.html"
- **port**: The port number that the server will bind to on the localhost. If omitted, 8080 will be used by default.

I organized this project into multiple source files so that each one has a clear responsibility:
- **server.c** contains the main function with the main server loop for accepting incoming connections and a signal handler for child process termination.
- **connection_handler.c** handles the HTTP request reading,parsing and response generation 
- **helpers.c** contains all the smaller utility functions that i wrote to abstract details and improve readability
- **transitions.c** contains all the transition functions of the request parser Finite State Machine and some test functions used in the transitions.
- **server.h**: Since this is not a large-sized project I chose to centralize most of the shared definitions/declarations into this single header file. This was done to achieve easier maintenance at the expense of each file not having access only to the information it needs.  



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
8) The parser stores all header field name/value pairs in a request
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
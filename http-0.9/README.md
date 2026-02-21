# An HTTP/0.9 server program

### Description

This project is a CLI program written in C which can be build with the "make" command according to the makefile included in the project. Default executable name is "server". It is a server program implementing the simple line HTTP/0.9 application layer protocol over TCP connections in the Internet Domain of communication (IP Addresses). I approached this project as practice in learning socket programming for Unix/Linux OS and the HTTP protocol. My ultimate goal in this series of projects is to combine all the knowledge i gained to complete a more sophisticated program which will be a partially HTTP/1.0 compliant server.

**Program Usage**: server root_dir [port].  
- **server**: the default executable file name
- **root_dir**: The directory that the server will use as its root for serving files. For example, the URL "/my_page.html" will correspond to the file stored on the server at "root_dir/my_page.html"
- **port**: The port number that the server will bind to on the localhost. If ommited, 8080 will be used by default.

Even though the project's size is relatively small, I divided it into multiple source files so that each one has a seperate semantic meaning and functionality; server.c contains the main function, parser.c contains the HTTP request parser and helpers.c contains all the smaller utility functions that i wrote to abstract away details from the others and imrove readability. Since this is a small project i placed all the shared definitions/declarations into a single header file, my_header.h, for easier maintenance.

### HTTP/0.9 request parser 

Modelled as a Deterministic Finite State Machine of the transducer type:

#### Specification

1) The parser system handles one HTTP/0.9 request.
2) The parser system responds to valid HTTP/0.9 requests with the entity-body of the Requested-URI
3) The parser system doesn't respond to invalid HTTP/0.9 requests.

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




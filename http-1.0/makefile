# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall 

SOURCES = server.c helpers.c parser.c transitions.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = server

# Default target
$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ 





# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall 

SOURCES = server.c helpers.c parser.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = server

# Default target
$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ 


.PHONY: clean

clean:
    @rm -f $(TARGET) $(OBJECTS) core


# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall 

SOURCES = echo.c helpers.c 
OBJECTS = $(SOURCES:.c=.o)
TARGET = echo

# Default target
$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ 


#!/bin/bash

# Set variables
CC=gcc
CFLAGS="-I./include/Linux/SDL2 -I./include/Linux/SDL2_ttf"
LDFLAGS="-L./lib/SDL2 -L./lib/SDL2_ttf -lSDL2main -lSDL2_ttf -lSDL2"

SRC="src/*.c"
OBJ="main.o"
EXEC="game"

# Create bin directory if it doesn't exist
mkdir -p bin

# Compile source files into object files
$CC -std=c17 -Wall -Wextra $CFLAGS -c $SRC -o bin/$OBJ
# Link object files into the executable
$CC -o bin/$EXEC bin/$OBJ $LDFLAGS
# Remove the object file
rm bin/$OBJ

# Change to the bin directory
cd bin
# Run the executable
./$EXEC
# Remove the executable after running
rm $EXEC
# Change back to the original directory
cd ..

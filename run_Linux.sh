#!/bin/bash

# Set variables
CC="gcc"
DFLAGS="-Wall -Wextra"
CFLAGS="-Iinclude/SDL2 -Iinclude/SDL2_ttf"
LDFLAGS="-Llib/SDL2 -Llib/SDL2_ttf -lSDL2 -lSDL2_ttf"

SRC="src/*.c"
OBJ="main.o"
EXEC="game"

# Create bin directory if it doesn't exist
mkdir -p bin

# Compile source files into object files
$CC $DFLAGS $CFLAGS -c $SRC -o $OBJ
# Link object files into the executable
$CC -o bin/$EXEC $OBJ $LDFLAGS
# Remove the object file
rm $OBJ

# Change to the bin directory
cd bin
# Run the executable
./$EXEC
# Remove the executable after running
rm $EXEC
# Change back to the original directory
cd ..
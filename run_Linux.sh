#!/bin/bash

# Set variables
EXEC="game"

# Create bin directory if it doesn't exist
mkdir -p bin

# Compiling code
gcc -std=c17 src/*.c -Wall -Wextra -o bin/$EXEC $(sdl2-config --cflags --libs)

# Change to the bin directory
cd bin
# Run the executable
./$EXEC
# Remove the executable after running
rm $EXEC
# Change back to the original directory
cd ..
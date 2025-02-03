@echo off

rem Set variables
set CC=gcc
set CFLAGS=-Iinclude\SDL2
set LDFLAGS=-Llib\SDL2 -Llib\SDL2_ttf -lmingw32 -lSDL2main -lSDL2

set SRC=src\*.c
set OBJ=main.o
set EXEC=game.exe

rem Create bin directory if it doesn't exist
if not exist bin mkdir bin

rem Compile source files into object files
%CC% -std=c17 -Wall -Wextra %CFLAGS% -c %SRC% -o bin\%OBJ%
rem Link object files into the executable
%CC% -o bin\%EXEC% bin\%OBJ% %LDFLAGS%
rem Remove the object file
del bin\%OBJ%

rem Change to the bin directory
cd bin
rem Run the executable
%EXEC%
rem Remove the executable after running
del %EXEC%
rem Change back to the original directory
cd ..
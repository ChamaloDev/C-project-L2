@echo off

rem Set variables
set CC=gcc
set DFLAGS=-Wall -Wextra
set CFLAGS=-Iinclude\SDL2 -Iinclude\SDL2_ttf
set LDFLAGS=-Llib\SDL2 -Llib\SDL2_ttf -lmingw32 -lSDL2main -lSDL2_ttf -lSDL2

set SRC=src\*.c
set OBJ=main.o
set EXEC=game.exe

rem Create bin directory if it doesn't exist
if not exist bin mkdir bin

rem Compile source files into object files
%CC% %DFLAGS% %CFLAGS% -c %SRC% -o %OBJ%
rem Link object files into the executable
%CC% -o bin\%EXEC% %OBJ% %LDFLAGS%
rem Remove the object file
del %OBJ%

rem Change to the bin directory
cd bin
rem Run the executable
%EXEC%
rem Remove the executable after running
del %EXEC%
rem Change back to the original directory
cd ..
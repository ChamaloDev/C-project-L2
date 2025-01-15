#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/SDL2/include/SDL2/SDL.h"


int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 0;
    }
    printf("SDL successfully initialized!\n");
    SDL_Quit();
    return 0;
}

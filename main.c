#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "src/SDL2/include/SDL2/SDL.h"
#define WIDTH 1280
#define HEIGHT 720
#define FULLSCREEN false
#define FPS 60
#define ROWS 7
#define COLLUMNS 21




typedef struct defence {
    int type;              // Defence type, determine it's abilities, look and upgrades
    int live_points;       // Live points of the defence, when it reaches 0 or bellow the defence is destroyed
    int row;               // Row number of the defence, 0 being the topmost row
    int collumn;           // Collumn number of the defence, 0 being the leftmost collumn
    int cost;              // Placement cost of the defence
    struct defence* next;  // Pointer to the next defence placed
} Defence;

typedef struct enemy {
    int type;                 // Enemy type, determine it's abilities and look
    int live_points;          // Live points of the enemy, when it reaches 0 or bellow the enemy is defeated
    int row;                  // Row number of the enemy, 0 being the topmost row
    int collumn;              // Collumn number of the enemy, 0 being the leftmost collumn
    int speed;                // Number of collumn travelled per turn
    int spawn_delay;          // Number of turn before spawning in
    struct enemy* next;       // Next enemy on the row
    struct enemy* next_line;  // First enemy in next row (bellow)
    struct enemy* prev_line;  // First enemy in previous row (above)
} Enemy;

typedef struct {
    Defence *defences;  // Defences
    Enemy *enemies;     // Enemies
    int funds;          // Availible funds to build defences
    int turn_nb;        // Turn number
} Game;




/* Return the minimum between x and y */
double min(double x, double y) {return (x < y) ? x : y;}

/* Return the maximum between x and y */
double max(double x, double y) {return (x > y) ? x : y;}

/* Add 2 strings together into a new string, memory must be freed after use */
char *concatString(const char *a, const char *b) {
    char *c = malloc((strlen(a) + strlen(b) + 1) * sizeof(char));
    strcpy(c, a);
    strcat(c, b);
    return c;
}




/* Load an image in bitmap (.bmp) format */
SDL_Surface *loadImg(const char *path) {
    char *full_path = concatString("./src/img/", path);
    SDL_Surface *img = SDL_LoadBMP(full_path);
    free(full_path);
    return img;
}

void delImg(SDL_Surface *img) {
    SDL_FreeSurface(img);
}

/* Draw an image on the window surface */
void drawImg(SDL_Renderer *rend, SDL_Surface *img, int center_x, int center_y, int width, int height) {
    /* Get current screen scaling (compared to original) */
    int wind_width, wind_height;
    SDL_GetWindowSizeInPixels(SDL_RenderGetWindow(rend), &wind_width, &wind_height);
    double scale_x = ((double) wind_width) / WIDTH, scale_y = ((double) wind_height) / HEIGHT;
    /* Destination area */
    SDL_Rect dest = {(center_x - img->w/2) * scale_x, (center_y - img->h/2) * scale_y, width * scale_x, height * scale_y};
    /* Convert surface to texture and draw it */
    SDL_Texture *sprite = SDL_CreateTextureFromSurface(rend, img);
    SDL_RenderCopy(rend, sprite, NULL, &dest);
    SDL_DestroyTexture(sprite);
}




int main(int argc, char* argv[]) {
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 0;
    }
    /* Create a window */
    SDL_Window* wind = SDL_CreateWindow("Game of the year", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!wind) {
        printf("Error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }
    SDL_SetWindowFullscreen(wind, SDL_WINDOW_FULLSCREEN_DESKTOP*FULLSCREEN);
    /* Create a renderer */
    Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    SDL_Renderer* rend = SDL_CreateRenderer(wind, -1, render_flags);
    if (!rend) {
        printf("Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(wind);
        SDL_Quit();
        return 0;
    }

    /* Load images */
    SDL_Surface *grass_tiles[] = {loadImg("others/grass_tile_a.bmp"), loadImg("others/grass_tile_b.bmp"), loadImg("others/grass_tile_c.bmp"), loadImg("others/grass_tile_d.bmp")};
    /* Main loop */
    bool fullscreen = FULLSCREEN;
    Uint64 tick = SDL_GetTicks64();
    SDL_Event event;
    bool running = true;
    while (running) {
        /* Process events */
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                /* Quit the game */
                case SDL_QUIT:
                    running = false;
                    break;
                /* Key pressed */
                case SDL_KEYDOWN:
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_ESCAPE:
                            running = false;
                            break;
                        case SDL_SCANCODE_F11:
                            fullscreen = !fullscreen;
                            SDL_SetWindowFullscreen(wind, SDL_WINDOW_FULLSCREEN_DESKTOP*fullscreen);
                            break;
                        default:
                            break;
                    }
                    break;
                /* Key released */
                case SDL_KEYUP:
                    switch (event.key.keysym.scancode) {
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
        /* Clear screen */
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);
        /* Draw elements */
        for (int y = 0; y < ROWS; y++) {
            for (int x = 0; x < COLLUMNS; x++) {
                drawImg(rend, grass_tiles[x%2 + (y%2) * 2], (WIDTH/COLLUMNS) * (x*2 + 5) / 2, (HEIGHT/ROWS) * (y*2 + 1) / 2, (13*WIDTH)/(10*COLLUMNS), (2*HEIGHT)/ROWS);
            }
        }
        /* Draw to window and loop */
        SDL_RenderPresent(rend);
        SDL_Delay(max(1000/FPS - (SDL_GetTicks64()-tick), 0));
        tick = SDL_GetTicks64();
    }

    /* Free allocated memory */
    delImg(grass_tiles[3]); delImg(grass_tiles[2]); delImg(grass_tiles[1]); delImg(grass_tiles[0]);
    /* Release resources */
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(wind);
    SDL_Quit();
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "src/SDL2/include/SDL2/SDL.h"
#define WIDTH 1280        // Default width of the window in px
#define HEIGHT 720        // Default height of the window in px
#define FULLSCREEN false  // Set if the game should start on fullscreen (F11 to toggle on/off)
#define FPS 60            // Game target FPS
#define ROWS 7            // Number of rows for the map
#define COLLUMNS 21       // Number of collumns for the map
#define TILE_W 256        // Width of a tile in px
#define TILE_H 192        // Heigh of a tile in px
#define SPRITE_W 320      // Width of all sprites in px
#define SPRITE_H 256      // Height of all sprites in px




/* Position of the camera */
double cam_scale = 1.0;
double cam_pos_x = 0.0;
double cam_pos_y = 0.0;




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
    Enemy *enemies;     // Enemies
    int income;         // Availible funds to build defences
} Wave;

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

/* Compute x to the power of n */
double pow(double x, int n) {
    /* Base case */
    if (n == 0) return 1;
    if (x == 0.0) return 0;
    /* Recursive case */
    if (n > 0) {
        double y = x;
        int i = 1;
        while (i*2 <= n) {
            y *= y;
            i *= 2;
        }
        return y * pow(x, n-i);
    }
    /* Negative exponents */
    else return 1.0 / pow(x, -n);
}

/* Add 2 strings together into a new string, memory must be freed after use */
char *concatString(const char *a, const char *b) {
    char *c = malloc((strlen(a) + strlen(b) + 1) * sizeof(char));
    strcpy(c, a);
    strcat(c, b);
    return c;
}




// [NOT IMPLEMENTED]
/* Read single file line */
bool readLine(FILE *file, char ***line, int *nb_values) {
    char line_buffer[32], value_buffer[8];
    if (!(fgets(line_buffer, 32, file))) return false;
    // TODO!
}

// [NOT IMPLEMENTED]
/* Load a level */
/* File must be located in "./src/lvl/<path>.txt" */
bool loadLevel(const char *path, Wave ***waves, int *nb_waves) {
    /* Openning text file */
    char *partial_path = concatString("./src/lvl/", path);
    char *full_path = concatString(partial_path, ".txt");
    FILE *file = fopen(full_path, "r");
    /* Checking if file was open successfully */
    if (!(file)) printf("[ERROR]    Level file at \"%s\" not found\n", full_path);
    free(partial_path);
    free(full_path);
    if (!(file)) return false;

    // TODO!
    return true;
}




/* Load an image in bitmap (.bmp) format */
/* File must be located in "./src/img/<path>.bmp" */
SDL_Surface *loadImg(const char *path) {
    char *partial_path = concatString("./src/img/", path);
    char *full_path = concatString(partial_path, ".bmp");
    SDL_Surface *img = SDL_LoadBMP(full_path);
    free(partial_path);
    free(full_path);
    return img;
}

void delImg(SDL_Surface *img) {
    SDL_FreeSurface(img);
}

/* Draw an image on the window surface */
void drawImg(SDL_Renderer *rend, SDL_Surface *img, int center_x, int center_y, int width, int height) {
    /* Destination area */
    SDL_Rect dest = {(center_x - width/2 - cam_pos_x) * cam_scale, (center_y - height/2 - cam_pos_y) * cam_scale, width * cam_scale, height * cam_scale};
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
    SDL_Surface *grass_tiles[] = {loadImg("others/grass_tile_a"), loadImg("others/grass_tile_b"), loadImg("others/grass_tile_c"), loadImg("others/grass_tile_d")};
    /* Main loop */
    bool mouse_dragging = false;
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
                /* Mouse button pressed */
                case SDL_MOUSEBUTTONDOWN:
                    switch (event.button.button) {
                        case SDL_BUTTON_RIGHT:
                            mouse_dragging = true;
                            break;
                        default:
                            break;
                    }
                    break;
                /* Mouse button released */
                case SDL_MOUSEBUTTONUP:
                    switch (event.button.button) {
                        case SDL_BUTTON_RIGHT:
                            mouse_dragging = false;
                            break;
                        default:
                            break;
                    }
                    break;
                /* Mouse wheel used */
                case SDL_MOUSEWHEEL:
                    cam_scale = min(max(0.2, cam_scale*pow(1.05, event.wheel.y)), 2.0);
                    break;
                /* Mouse moved */
                case SDL_MOUSEMOTION:
                    if (mouse_dragging) {
                        cam_pos_x -= event.motion.xrel / cam_scale;
                        cam_pos_y -= event.motion.yrel / cam_scale;
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
                drawImg(rend, grass_tiles[x%2 + (y%2) * 2], TILE_W * (x*2 + 1)/2, TILE_H * (y*2 + 1)/2, SPRITE_W, SPRITE_H);
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
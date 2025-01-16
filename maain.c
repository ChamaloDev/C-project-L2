#include <stdio.h>
#include <stdbool.h>
#include "src/SDL2/include/SDL2/SDL.h"
#define WIDTH 640
#define HEIGHT 480
#define SIZE 200
#define SPEED 600
#define GRAVITY 60
#define FPS 60
#define JUMP -1200

typedef struct tourelle{
	int type;
	int pointsDeVie;
	int ligne;
	int position;
	int prix;
	struct tourelle* next;
} Tourelle;

typedef struct etudiant{
	int type;
	int pointsDeVie;
	int ligne;
	int position;
	int vitesse;
	int tour;
	struct etudiant* next;
	struct etudiant* next_line;
	struct etudiant* prev_line;
} Etudiant;

typedef struct{
	Tourelle* tourelles;
	Etudiant* etudiants;
	int cagnotte;
	int tour;
} Jeu;

int main(int argc,char *argv[]){
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
    	printf("Error initializing SDL: %s\n", SDL_GetError());
    	return 1;
  	}
  	bool running = true,fullscreen = false;
  	SDL_Rect rect = {WIDTH,HEIGHT,SIZE,SIZE};
  	SDL_Event event;
  	SDL_Window* wind = SDL_CreateWindow("Tower defense 2 ouf", SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WIDTH,HEIGHT,SDL_WINDOW_SHOWN);
  	while (running){
	  	while (SDL_PollEvent(&event)){
	  		switch(event.type){
	  			case SDL_QUIT:
	  				running =false;
	  				break;
	  			case SDL_KEYUP:
	  				if (event.key.keysym.sym == SDLK_f){
	  					if (fullscreen ==false){
	  						fullscreen = true;
	  						SDL_SetWindowFullscreen(wind,SDL_WINDOW_FULLSCREEN);
	  					}
	  					else if ( fullscreen ==true){
	  						fullscreen =false;
	  						SDL_SetWindowFullscreen(wind,0);
	  					}
	  				
	  				}
	  				break;
	  		}
	  	}
	}
	SDL_DestroyWindow(wind)
  	SDL_Quit();
  	return 0;
}
